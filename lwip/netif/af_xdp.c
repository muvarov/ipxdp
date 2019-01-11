#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"

#include "af_xdp.h"

#include <sys/ioctl.h>
#include <linux/if.h>

#ifndef AFXDP_DEFAULT_IF
#define AFXDP_DEFAULT_IF "eth0"
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 'a'
#define IFNAME1 'f'

#ifndef AFXDPIF_DEBUG
#define AFXDPIF_DEBUG LWIP_DBG_ON
#endif

typedef struct af_xdp_if {
  /* Add whatever per-interface state that is needed here. */
} af_xdp_if;

/* Forward declarations. */
static void af_xdp_if_input(struct netif *netif);
#if !NO_SYS
static void af_xdp_if_thread(void *arg);
#endif /* !NO_SYS */

/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif)
{
  struct af_xdp_if *af_xdp_if;
#if LWIP_IPV4
  int ret;
  char buf[1024];
#endif /* LWIP_IPV4 */
  char *preconfigured_tapif = getenv("PRECONFIGURED_AF_XDP_IF");

  af_xdp_if = (struct af_xdp_if*)netif->state;

  /* Obtain MAC address from network interface. */

  /* (We just fake an address...) */
  netif->hwaddr[0] = 0x02;
  netif->hwaddr[1] = 0x12;
  netif->hwaddr[2] = 0x34;
  netif->hwaddr[3] = 0x56;
  netif->hwaddr[4] = 0x78;
  netif->hwaddr[5] = 0xab;
  netif->hwaddr_len = 6;

  /* device capabilities */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  netif_set_link_up(netif);

#if !NO_SYS
  sys_thread_new("af_xdp_if_thread", af_xdp_if_thread, netif, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
#endif /* !NO_SYS */
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct af_xdp_if *af_xdp_if = (struct af_xdp_if*)netif->state;
  char buf[1518]; /* max packet size including VLAN excluding CRC */
  ssize_t written;

#if 0
  if (((double)rand()/(double)RAND_MAX) < 0.2) {
    printf("drop output\n");
    return ERR_OK; /* ERR_OK because we simulate packet loss on cable */
  }
#endif

  if (p->tot_len > sizeof(buf)) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("af_xdp_if: packet too large");
    return ERR_IF;
  }

  /* initiate transfer(); */
  pbuf_copy_partial(p, buf, p->tot_len, 0);


  /* signal that packet should be sent(); */
 // written = write(tapif->fd, buf, p->tot_len);
  written = p->tot_len;

  if (written < p->tot_len) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("tapif: write");
    return ERR_IF;
  } else {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, (u32_t)written);
    return ERR_OK;
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *netif)
{
  struct pbuf *p;
  u16_t len;
  ssize_t readlen;
  char buf[1518]; /* max packet size including VLAN excluding CRC */
  struct af_xdp_if *af_xdp_if = (struct af_xdp_if*)netif->state;

  /* Max: add read here */

  len = 0;

  MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);

#if 0
  /* Simulate drop on input */
  if (((double)rand()/(double)RAND_MAX) < 0.2) {
    printf("drop\n");
    return NULL;
  }
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL) {
    pbuf_take(p, buf, len);
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    LWIP_DEBUGF(NETIF_DEBUG, ("tapif_input: could not allocate pbuf\n"));
  }

  return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * tapif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
af_xdp_if_input(struct netif *netif)
{
  struct pbuf *p = low_level_input(netif);

  if (p == NULL) {
#if LINK_STATS
    LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
    LWIP_DEBUGF(AFXDPIF_DEBUG, ("af_xdp_if_input: low_level_input returned NULL\n"));
    return;
  }

  if (netif->input(p, netif) != ERR_OK) {
    LWIP_DEBUGF(NETIF_DEBUG, ("af_xdp_if_input: netif input error\n"));
    pbuf_free(p);
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * af_xdp_if_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
af_xdp_if_init(struct netif *netif)
{
  struct af_xdp_if *af_xdp_if;

  af_xdp_if = (struct af_xdp_if *)mem_malloc(sizeof(struct af_xdp_if));
  if (af_xdp_if == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("tapif_init: out of memory for tapif\n"));
    return ERR_MEM;
  }
  netif->state = af_xdp_if;
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
  netif->linkoutput = low_level_output;
  netif->mtu = 1500;

  low_level_init(netif);

  return ERR_OK;
}


/*-----------------------------------------------------------------------------------*/
void
af_xdp_if_poll(struct netif *netif)
{
  printf("%s()\n", __func__);
  af_xdp_if_input(netif);
}

#if NO_SYS

int
af_xpd_if_select(struct netif *netif)
{
  printf("%s()\n", __func__);
  return 0;
}

#else /* NO_SYS */

static void
af_xdp_if_thread(void *arg)
{
  struct netif *netif;
  struct af_xdp_if  *af_xdp_if;
  fd_set fdset;
  int ret;

  netif = (struct netif *)arg;
  af_xdp_if = (struct af_xdp_if *)netif->state;
  printf("%s() running\n", __func__);

  while(1) {
    /* Wait for a packet to arrive. */
    //ret = select(tapif->fd + 1, &fdset, NULL, NULL, NULL);

    if(ret == 1) {
      /* Handle incoming packet. */
      af_xdp_if_input(netif);
    } else if(ret == -1) {
      perror("af_xdp_thread: select");
    }
  }
}

#endif /* NO_SYS */

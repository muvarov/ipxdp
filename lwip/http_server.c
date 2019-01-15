#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/api.h"

/* network config */
#include "lwipcfg.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "lwip/netif.h"
#include "lwip/api.h"
#include <netif/af_xdp.h>

#include <unistd.h>

static struct netif netif;

extern void http_server_netconn_init(void);

#if LWIP_IPV4
#define NETIF_ADDRS ipaddr, netmask, gw,
void init_default_netif(const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw)
#else
#define NETIF_ADDRS
void init_default_netif(void)
#endif
{
#if NO_SYS
	netif_add(&netif, NETIF_ADDRS NULL, tapif_init, netif_input);
#else
	//netif_add(&netif, NETIF_ADDRS NULL, tapif_init, tcpip_input);
	netif_add(&netif, NETIF_ADDRS NULL, af_xdp_if_init, tcpip_input);
	printf("added default netif af_xdp_if_init\n");
#endif
  	netif_set_default(&netif);
}

/* This function initializes all network interfaces */
static void test_netif_init(void)
{
	ip4_addr_t ipaddr, netmask, gw;
	err_t err;

	ip4_addr_set_zero(&gw);
	ip4_addr_set_zero(&ipaddr);
	ip4_addr_set_zero(&netmask);

	LWIP_PORT_INIT_GW(&gw);
	LWIP_PORT_INIT_IPADDR(&ipaddr);
	LWIP_PORT_INIT_NETMASK(&netmask);
	printf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));

	init_default_netif(&ipaddr, &netmask, &gw);

#if LWIP_NETIF_STATUS_CALLBACK
	netif_set_status_callback(netif_default, status_callback);
#endif
#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(netif_default, link_callback);
#endif

	netif_set_up(netif_default);
	printf("%s() done.\n", __func__);
}

static void test_init(void * arg)
{
#if NO_SYS
	LWIP_UNUSED_ARG(arg);
#else /* NO_SYS */
	sys_sem_t *init_sem;
	LWIP_ASSERT("arg != NULL", arg != NULL);
	init_sem = (sys_sem_t*)arg;
#endif /* NO_SYS */

	test_netif_init();

#if !NO_SYS
	sys_sem_signal(init_sem);
#endif /* !NO_SYS */
}

int main(void)
{
	const ip_addr_t ping_addr;
	sys_sem_t init_sem;
	err_t err;

	err = sys_sem_new(&init_sem, 0);
	LWIP_ASSERT("failed to create init_sem", err == ERR_OK);
	tcpip_init(test_init, &init_sem);
	/* we have to wait for initialization to finish before
	 * calling update_adapter()! */
	sys_sem_wait(&init_sem);
	sys_sem_free(&init_sem);

	http_server_netconn_init();

	while(1)
		sleep(100);
	return 0;
}


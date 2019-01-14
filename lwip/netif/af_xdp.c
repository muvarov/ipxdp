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

#include <assert.h>
#include <libgen.h>
#include <linux/bpf.h>
#include <linux/if_link.h>
#include <linux/if_xdp.h>
#include <linux/if_ether.h>
#include <bpf/libbpf.h>
#include <bpf_util.h>
#include <bpf/bpf.h>
#include <net/ethernet.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <time.h>

#include <bpf/libbpf.h>
#include <bpf_util.h>
#include <bpf/bpf.h>

#ifndef AFXDP_DEFAULT_IF
#define AFXDP_DEFAULT_IF "enp8s0"
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 'a'
#define IFNAME1 'f'

#ifndef AFXDPIF_DEBUG
#define AFXDPIF_DEBUG LWIP_DBG_ON
#endif

struct af_xdp_if {
  /* Add whatever per-interface state that is needed here. */
	int num_socks;
	struct xdpsock *xsks[MAX_SOCKS];
	int opt_queue;
	int if_idx;
};

struct xdp_umem_uqueue {
	u32 cached_prod;
	u32 cached_cons;
	u32 mask;
	u32 size;
	u32 *producer;
	u32 *consumer;
	u64 *ring;
	void *map;
};

struct xdp_umem {
	char *frames;
	struct xdp_umem_uqueue fq;
	struct xdp_umem_uqueue cq;
	int fd;
};

struct xdp_uqueue {
	u32 cached_prod;
	u32 cached_cons;
	u32 mask;
	u32 size;
	u32 *producer;
	u32 *consumer;
	struct xdp_desc *ring;
	void *map;
};

struct xdpsock {
	struct xdp_uqueue rx;
	struct xdp_uqueue tx;
	int sfd;
	struct xdp_umem *umem;
	u32 outstanding_tx;
	unsigned long rx_npkts;
	unsigned long tx_npkts;
	unsigned long prev_rx_npkts;
	unsigned long prev_tx_npkts;
};

/* Forward declarations. */
static struct xdpsock *xsk_configure(struct xdp_umem *umem, struct af_xdp_if *af_xdp_if);
static u32 xq_nb_avail(struct xdp_uqueue *q, u32 ndescs);
static int umem_fill_to_kernel_ex(struct xdp_umem_uqueue *fq,
				  struct xdp_desc *d,
				  size_t nb);

static void af_xdp_if_input(struct netif *netif);
#if !NO_SYS
static void af_xdp_if_thread(void *arg);
#endif /* !NO_SYS */

static void hex_dump(void *pkt, size_t length, u64 addr)
{
	const unsigned char *address = (unsigned char *)pkt;
	const unsigned char *line = address;
	size_t line_size = 32;
	unsigned char c;
	char buf[32];
	int i = 0;

	if (!DEBUG_HEXDUMP)
		return;

	sprintf(buf, "addr=%lu", addr);
	printf("length = %zu\n", length);
	printf("%s | ", buf);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");	/* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", buf);
		}
	}
	printf("\n");
}

/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif)
{
	struct af_xdp_if *af_xdp_if = (struct af_xdp_if*)netif->state;
	struct rlimit r = {RLIM_INFINITY, RLIM_INFINITY};
	struct bpf_prog_load_attr prog_load_attr = {
		.prog_type	= BPF_PROG_TYPE_XDP,
	};
	int prog_fd, qidconf_map, xsks_map;
	struct bpf_object *obj;
	char xdp_filename[256];
	struct bpf_map *map;
	int i, ret, key = 0;

	if (setrlimit(RLIMIT_MEMLOCK, &r)) {
		fprintf(stderr, "ERROR: setrlimit(RLIMIT_MEMLOCK) \"%s\"\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	snprintf(xdp_filename, sizeof(xdp_filename), "%s_kern.o", "lwip_af_xdp");
	prog_load_attr.file = xdp_filename;

	if (bpf_prog_load_xattr(&prog_load_attr, &obj, &prog_fd))
		exit(EXIT_FAILURE);
	if (prog_fd < 0) {
		fprintf(stderr, "ERROR: no program found: %s\n",
			strerror(prog_fd));
		exit(EXIT_FAILURE);
	}

	map = bpf_object__find_map_by_name(obj, "qidconf_map");
	qidconf_map = bpf_map__fd(map);
	if (qidconf_map < 0) {
		fprintf(stderr, "ERROR: no qidconf map found: %s\n",
			strerror(qidconf_map));
		exit(EXIT_FAILURE);
	}

	map = bpf_object__find_map_by_name(obj, "xsks_map");
	xsks_map = bpf_map__fd(map);
	if (xsks_map < 0) {
		fprintf(stderr, "ERROR: no xsks map found: %s\n",
			strerror(xsks_map));
		exit(EXIT_FAILURE);
	}

	af_xdp_if->if_idx = if_nametoindex(AFXDP_DEFAULT_IF); /* eth0*/
	if (bpf_set_link_xdp_fd(af_xdp_if->if_idx, prog_fd, 0 /*opt_xdp_flags*/) < 0) {
		fprintf(stderr, "ERROR: link %s set xdp fd %d failed\n",
			AFXDP_DEFAULT_IF, af_xdp_if->if_idx);
		exit(EXIT_FAILURE);
	}

	af_xdp_if->opt_queue = 0;
	ret = bpf_map_update_elem(qidconf_map, &key, &af_xdp_if->opt_queue, 0);
	if (ret) {
		fprintf(stderr, "ERROR: bpf_map_update_elem qidconf\n");
		exit(EXIT_FAILURE);
	}


	/* Create sockets... */
	af_xdp_if->xsks[af_xdp_if->num_socks++] = xsk_configure(NULL, af_xdp_if);

#if 0 || RR_LB
	for (i = 0; i < MAX_SOCKS - 1; i++)
		xsks[af_xdp_if->num_socks++] = xsk_configure(af_xdp_if->xsks[0]->umem, af_xdp_if);
#endif

	/* ...and insert them into the map. */
	for (i = 0; i < af_xdp_if->num_socks; i++) {
		key = i;
		ret = bpf_map_update_elem(xsks_map, &key, &af_xdp_if->xsks[i]->sfd, 0);
		if (ret) {
			fprintf(stderr, "ERROR: bpf_map_update_elem %d\n", i);
			exit(EXIT_FAILURE);
		}
	}

	/* Configure LWIP things... */

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

	printf("af_xdp: link %s set xdp fd %d ok.\n",
	       AFXDP_DEFAULT_IF, af_xdp_if->if_idx);

#if !NO_SYS
	sys_thread_new("af_xdp_if_thread", af_xdp_if_thread, netif, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
#endif /* !NO_SYS */
}

static inline u32 xq_nb_free(struct xdp_uqueue *q, u32 ndescs)
{
	u32 free_entries = q->cached_cons - q->cached_prod;

	if (free_entries >= ndescs)
		return free_entries;

	/* Refresh the local tail pointer */
	q->cached_cons = *q->consumer + q->size;
	return q->cached_cons - q->cached_prod;
}

static void kick_tx(int fd)
{
	int ret;

	ret = sendto(fd, NULL, 0, MSG_DONTWAIT, NULL, 0);
	if (ret >= 0 || errno == ENOBUFS || errno == EAGAIN || errno == EBUSY)
		return;
	lassert(0);
}

static inline u32 umem_nb_avail(struct xdp_umem_uqueue *q, u32 nb)
{
	u32 entries = q->cached_prod - q->cached_cons;

	if (entries == 0) {
		q->cached_prod = *q->producer;
		entries = q->cached_prod - q->cached_cons;
	}

	return (entries > nb) ? nb : entries;
}

static inline size_t umem_complete_from_kernel(struct xdp_umem_uqueue *cq,
					       u64 *d, size_t nb)
{
	u32 idx, i, entries = umem_nb_avail(cq, nb);

	u_smp_rmb();

	for (i = 0; i < entries; i++) {
		idx = cq->cached_cons++ & cq->mask;
		d[i] = cq->ring[idx];
	}

	if (entries > 0) {
		u_smp_wmb();

		*cq->consumer = cq->cached_cons;
	}

	return entries;
}

static inline void *xq_get_data(struct xdpsock *xsk, u64 addr)
{
	return &xsk->umem->frames[addr];
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
	struct xdp_desc descs[BATCH_SIZE];
	struct xdp_uqueue *uq = &af_xdp_if->xsks[0]->tx; 
	struct xdp_desc *r = uq->ring;
	u32 idx;
	unsigned int tx_idx = 0;
	unsigned int rcvd;

	printf("%s plen %d\n", __func__, p->tot_len);
#if 0
	if (((double)rand()/(double)RAND_MAX) < 0.2) {
		printf("drop output\n");
		return ERR_OK; /* ERR_OK because we simulate packet loss on cable */
	}
#endif

	if (xq_nb_free(uq, 1 /*ndescs*/) < 1) {
		MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
		perror("xq_nb_free");
		return ERR_IF;
	}

	idx = uq->cached_prod++ & uq->mask;
	r[idx].addr = tx_idx << FRAME_SHIFT;
	pbuf_copy_partial(p,
			  xq_get_data(af_xdp_if->xsks[0], r[idx].addr),
			  p->tot_len, 0); /*fixme: need zero-copy*/
	r[idx].len = p->tot_len;

	u_smp_wmb();

	*uq->producer = uq->cached_prod;

	kick_tx(af_xdp_if->xsks[0]->sfd);

	rcvd = umem_complete_from_kernel(&af_xdp_if->xsks[0]->umem->cq, descs, BATCH_SIZE);
	if (rcvd > 0) {
		af_xdp_if->xsks[0]->outstanding_tx -= rcvd;
		af_xdp_if->xsks[0]->tx_npkts += rcvd;
	}

	MIB2_STATS_NETIF_ADD(netif, ifoutoctets, descs[0].len);
	return ERR_OK;
}

static inline int xq_deq(struct xdp_uqueue *uq,
			 struct xdp_desc *descs,
			 int ndescs)
{
	struct xdp_desc *r = uq->ring;
	unsigned int idx;
	int i, entries;

	entries = xq_nb_avail(uq, ndescs);

	u_smp_rmb();

	for (i = 0; i < entries; i++) {
		idx = uq->cached_cons++ & uq->mask;
		descs[i] = r[idx];
	}

	if (entries > 0) {
		u_smp_wmb();

		*uq->consumer = uq->cached_cons;
	}

	return entries;
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
	struct af_xdp_if *af_xdp_if = (struct af_xdp_if*)netif->state;
	struct xdp_desc descs[BATCH_SIZE];
	unsigned int rcvd, i;
	struct xdpsock *xsk = af_xdp_if->xsks[0]; /*fixme: limitation for 1 socket */

	printf("%s\n", __func__);

	rcvd = xq_deq(&xsk->rx, descs, 1 /*BATCH_SIZE*/);
	if (!rcvd)
		return NULL;

	for (i = 0; i < rcvd; i++) {
		char *pkt = xq_get_data(xsk, descs[i].addr);

		hex_dump(pkt, descs[i].len, descs[i].addr);

		MIB2_STATS_NETIF_ADD(netif, ifinoctets, descs[i].len);

		/* We allocate a pbuf chain of pbufs from the pool. */
		p = pbuf_alloc(PBUF_RAW, descs[i].len, PBUF_POOL);
		if (p != NULL) {
			/* acknowledge that packet has been read(); */
			pbuf_take(p, (const void*)descs[i].addr, descs[i].len);
		} else {
			/* drop packet(); */
			MIB2_STATS_NETIF_INC(netif, ifindiscards);
			LWIP_DEBUGF(NETIF_DEBUG, ("tapif_input: could not allocate pbuf\n"));
		}
	}


	umem_fill_to_kernel_ex(&xsk->umem->fq, descs, rcvd);

#if 0
	/* Simulate drop on input */
	if (((double)rand()/(double)RAND_MAX) < 0.2) {
		printf("drop\n");
		return NULL;
	}
#endif

	return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * af_xdp_if_input():
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

  printf("%s() done.\n", __func__);
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

static unsigned long prev_time;

static unsigned long get_nsecs(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000UL + ts.tv_nsec;
}

static void dump_stats(struct af_xdp_if *af_xdp_if)
{
	unsigned long now = get_nsecs();
	long dt = now - prev_time;
	int i;
	struct xdpsock **xsks = af_xdp_if->xsks;

	prev_time = now;

	for (i = 0; i < af_xdp_if->num_socks && xsks[i]; i++) {
		char *fmt = "%-15s %'-11.0f %'-11lu\n";
		double rx_pps, tx_pps;

		rx_pps = (xsks[i]->rx_npkts - xsks[i]->prev_rx_npkts) *
			 1000000000. / dt;
		tx_pps = (xsks[i]->tx_npkts - xsks[i]->prev_tx_npkts) *
			 1000000000. / dt;

		printf("\n sock%d@", i);
		//print_benchmark(false);
		printf("\n");

		printf("%-15s %-11s %-11s %-11.2f\n", "", "pps", "pkts",
		       dt / 1000000000.);
		printf(fmt, "rx", rx_pps, xsks[i]->rx_npkts);
		printf(fmt, "tx", tx_pps, xsks[i]->tx_npkts);

		xsks[i]->prev_rx_npkts = xsks[i]->rx_npkts;
		xsks[i]->prev_tx_npkts = xsks[i]->tx_npkts;
	}
}

static inline u32 umem_nb_free(struct xdp_umem_uqueue *q, u32 nb)
{
	u32 free_entries = q->cached_cons - q->cached_prod;

	if (free_entries >= nb)
		return free_entries;

	/* Refresh the local tail pointer */
	q->cached_cons = *q->consumer + q->size;

	return q->cached_cons - q->cached_prod;
}

static u32 xq_nb_avail(struct xdp_uqueue *q, u32 ndescs)
{
	u32 entries = q->cached_prod - q->cached_cons;

	if (entries == 0) {
		q->cached_prod = *q->producer;
		entries = q->cached_prod - q->cached_cons;
	}

	return (entries > ndescs) ? ndescs : entries;
}

static int umem_fill_to_kernel_ex(struct xdp_umem_uqueue *fq,
					 struct xdp_desc *d,
					 size_t nb)
{
	u32 i;

	if (umem_nb_free(fq, nb) < nb)
		return -ENOSPC;

	for (i = 0; i < nb; i++) {
		u32 idx = fq->cached_prod++ & fq->mask;

		fq->ring[idx] = d[i].addr;
	}

	u_smp_wmb();

	*fq->producer = fq->cached_prod;

	return 0;
}

static inline int umem_fill_to_kernel(struct xdp_umem_uqueue *fq, u64 *d,
				      size_t nb)
{
	u32 i;

	if (umem_nb_free(fq, nb) < nb)
		return -ENOSPC;

	for (i = 0; i < nb; i++) {
		u32 idx = fq->cached_prod++ & fq->mask;

		fq->ring[idx] = d[i];
	}

	u_smp_wmb();

	*fq->producer = fq->cached_prod;

	return 0;
}

static struct xdp_umem *xdp_umem_configure(int sfd)
{
	int fq_size = FQ_NUM_DESCS, cq_size = CQ_NUM_DESCS;
	struct xdp_mmap_offsets off;
	struct xdp_umem_reg mr;
	struct xdp_umem *umem;
	socklen_t optlen;
	void *bufs;

	umem = calloc(1, sizeof(*umem));
	lassert(umem);

	lassert(posix_memalign(&bufs, getpagesize(), /* PAGE_SIZE aligned */
			       NUM_FRAMES * FRAME_SIZE) == 0);

	mr.addr = (__u64)bufs;
	mr.len = NUM_FRAMES * FRAME_SIZE;
	mr.chunk_size = FRAME_SIZE;
	mr.headroom = FRAME_HEADROOM;

	lassert(setsockopt(sfd, SOL_XDP, XDP_UMEM_REG, &mr, sizeof(mr)) == 0);
	lassert(setsockopt(sfd, SOL_XDP, XDP_UMEM_FILL_RING, &fq_size,
			   sizeof(int)) == 0);
	lassert(setsockopt(sfd, SOL_XDP, XDP_UMEM_COMPLETION_RING, &cq_size,
			   sizeof(int)) == 0);

	optlen = sizeof(off);
	lassert(getsockopt(sfd, SOL_XDP, XDP_MMAP_OFFSETS, &off,
			   &optlen) == 0);

	umem->fq.map = mmap(0, off.fr.desc +
			    FQ_NUM_DESCS * sizeof(u64),
			    PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_POPULATE, sfd,
			    XDP_UMEM_PGOFF_FILL_RING);
	lassert(umem->fq.map != MAP_FAILED);

	umem->fq.mask = FQ_NUM_DESCS - 1;
	umem->fq.size = FQ_NUM_DESCS;
	umem->fq.producer = umem->fq.map + off.fr.producer;
	umem->fq.consumer = umem->fq.map + off.fr.consumer;
	umem->fq.ring = umem->fq.map + off.fr.desc;
	umem->fq.cached_cons = FQ_NUM_DESCS;

	umem->cq.map = mmap(0, off.cr.desc +
			     CQ_NUM_DESCS * sizeof(u64),
			     PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_POPULATE, sfd,
			     XDP_UMEM_PGOFF_COMPLETION_RING);
	lassert(umem->cq.map != MAP_FAILED);

	umem->cq.mask = CQ_NUM_DESCS - 1;
	umem->cq.size = CQ_NUM_DESCS;
	umem->cq.producer = umem->cq.map + off.cr.producer;
	umem->cq.consumer = umem->cq.map + off.cr.consumer;
	umem->cq.ring = umem->cq.map + off.cr.desc;

	umem->frames = bufs;
	umem->fd = sfd;

	return umem;
}

static struct xdpsock *xsk_configure(struct xdp_umem *umem, struct af_xdp_if *af_xdp_if)
{
	struct sockaddr_xdp sxdp = {};
	struct xdp_mmap_offsets off;
	int sfd, ndescs = NUM_DESCS;
	struct xdpsock *xsk;
	bool shared = true;
	socklen_t optlen;
	u64 i;

	sfd = socket(PF_XDP, SOCK_RAW, 0);
	lassert(sfd >= 0);

	xsk = calloc(1, sizeof(*xsk));
	lassert(xsk);

	xsk->sfd = sfd;
	xsk->outstanding_tx = 0;

	if (!umem) {
		shared = false;
		xsk->umem = xdp_umem_configure(sfd);
	} else {
		xsk->umem = umem;
	}

	lassert(setsockopt(sfd, SOL_XDP, XDP_RX_RING,
			   &ndescs, sizeof(int)) == 0);
	lassert(setsockopt(sfd, SOL_XDP, XDP_TX_RING,
			   &ndescs, sizeof(int)) == 0);
	optlen = sizeof(off);
	lassert(getsockopt(sfd, SOL_XDP, XDP_MMAP_OFFSETS, &off,
			   &optlen) == 0);

	/* Rx */
	xsk->rx.map = mmap(NULL,
			   off.rx.desc +
			   NUM_DESCS * sizeof(struct xdp_desc),
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED | MAP_POPULATE, sfd,
			   XDP_PGOFF_RX_RING);
	lassert(xsk->rx.map != MAP_FAILED);

	if (!shared) {
		for (i = 0; i < NUM_DESCS * FRAME_SIZE; i += FRAME_SIZE)
			lassert(umem_fill_to_kernel(&xsk->umem->fq, &i, 1)
				== 0);
	}

	/* Tx */
	xsk->tx.map = mmap(NULL,
			   off.tx.desc +
			   NUM_DESCS * sizeof(struct xdp_desc),
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED | MAP_POPULATE, sfd,
			   XDP_PGOFF_TX_RING);
	lassert(xsk->tx.map != MAP_FAILED);

	xsk->rx.mask = NUM_DESCS - 1;
	xsk->rx.size = NUM_DESCS;
	xsk->rx.producer = xsk->rx.map + off.rx.producer;
	xsk->rx.consumer = xsk->rx.map + off.rx.consumer;
	xsk->rx.ring = xsk->rx.map + off.rx.desc;

	xsk->tx.mask = NUM_DESCS - 1;
	xsk->tx.size = NUM_DESCS;
	xsk->tx.producer = xsk->tx.map + off.tx.producer;
	xsk->tx.consumer = xsk->tx.map + off.tx.consumer;
	xsk->tx.ring = xsk->tx.map + off.tx.desc;
	xsk->tx.cached_cons = NUM_DESCS;

	sxdp.sxdp_family = PF_XDP;
	sxdp.sxdp_ifindex = af_xdp_if->if_idx;  //opt_ifindex;
	sxdp.sxdp_queue_id = af_xdp_if->opt_queue; //opt_queue;

	if (shared) {
		sxdp.sxdp_flags = XDP_SHARED_UMEM;
		sxdp.sxdp_shared_umem_fd = umem->fd;
	} else {
		sxdp.sxdp_flags = 0; //opt_xdp_bind_flags;
	}

	lassert(bind(sfd, (struct sockaddr *)&sxdp, sizeof(sxdp)) == 0);

	return xsk;
}


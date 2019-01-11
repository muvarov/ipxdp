#ifndef LWIP_AF_XDP_IF_H
#define LWIP_AF_XDP_IF_H

#include <lwip/netif.h>

err_t af_xdp_if_init(struct netif *netif);
void af_xdp_poll(struct netif *netif);

#if NO_SYS
int af_xdp_select(struct netif *netif);
#endif /* NO_SYS */

/* private */
#define FQ_NUM_DESCS 1024
#define CQ_NUM_DESCS 1024
#define NUM_DESCS 1024

/* Power-of-2 number of sockets */
#define MAX_SOCKS 4

/* Round-robin receive */
#define RR_LB 0

#define NUM_FRAMES 131072
#define FRAME_SIZE 2048
#define FRAME_HEADROOM 0

#ifndef SOL_XDP
#define SOL_XDP 283
#endif

#ifndef AF_XDP
#define AF_XDP 44
#endif

#ifndef PF_XDP
#define PF_XDP AF_XDP
#endif

#define barrier() __asm__ __volatile__("": : :"memory")
#ifdef __aarch64__
#define u_smp_rmb() __asm__ __volatile__("dmb ishld": : :"memory")
#define u_smp_wmb() __asm__ __volatile__("dmb ishst": : :"memory")
#else
#define u_smp_rmb() barrier()
#define u_smp_wmb() barrier()
#endif
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static void dump_stats(void);

#define lassert(expr)							\
	do {								\
		if (!(expr)) {						\
			fprintf(stderr, "%s:%s:%i: Assertion failed: "	\
				#expr ": errno: %d/\"%s\"\n",		\
				__FILE__, __func__, __LINE__,		\
				errno, strerror(errno));		\
			dump_stats();					\
			exit(EXIT_FAILURE);				\
		}							\
	} while (0)



#endif /* LWIP_AF_XDP_IF_H */

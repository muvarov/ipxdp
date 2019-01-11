#ifndef LWIP_AF_XDP_IF_H
#define LWIP_AF_XDP_IF_H

#include <lwip/netif.h>

err_t af_xdp_if_init(struct netif *netif);
void af_xdp_poll(struct netif *netif);

#if NO_SYS
int af_xdp_select(struct netif *netif);
#endif /* NO_SYS */

#endif /* LWIP_AF_XDP_IF_H */

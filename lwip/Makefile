LWIPDIR=/opt/Linaro/xdp/lwip.git/src

include $(LWIPDIR)/Filelists.mk

LWIPUNIXPORT = $(LWIPDIR)/../contrib/ports/unix/port/perf.c \
		$(LWIPDIR)/../contrib/ports/unix/port/sys_arch.c \
		$(LWIPDIR)/../contrib/ports/unix/port/netif/sio.c \
		$(LWIPDIR)/../contrib/ports/unix/port/netif/fifo.c \
		$(LWIPDIR)/../contrib/ports/unix/port/netif/tapif.c

lwipnetif_SRCS = \
    ${LWIPDIR}/netif/ethernet.c     \
    ${LWIPDIR}/netif/bridgeif.c     \
    ${LWIPDIR}/netif/bridgeif_fdb.c

lwipapi_SRCS = \
    ${LWIPDIR}/api/api_lib.c  \
    ${LWIPDIR}/api/api_msg.c  \
    ${LWIPDIR}/api/err.c      \
    ${LWIPDIR}/api/if_api.c   \
    ${LWIPDIR}/api/netbuf.c   \
    ${LWIPDIR}/api/netdb.c    \
    ${LWIPDIR}/api/netifapi.c \
    ${LWIPDIR}/api/sockets.c  \
    ${LWIPDIR}/api/tcpip.c    \

lwipcore4_SRCS = \
    ${LWIPDIR}/core/ipv4/acd.c      \
    ${LWIPDIR}/core/ipv4/autoip.c   \
    ${LWIPDIR}/core/ipv4/dhcp.c     \
    ${LWIPDIR}/core/ipv4/etharp.c   \
    ${LWIPDIR}/core/ipv4/icmp.c     \
    ${LWIPDIR}/core/ipv4/igmp.c     \
    ${LWIPDIR}/core/ipv4/ip4_frag.c \
    ${LWIPDIR}/core/ipv4/ip4.c      \
    ${LWIPDIR}/core/ipv4/ip4_addr.c

example_app = $(LWIPDIR)/../contrib/apps/httpserver/httpserver-netconn.c \
		http_server.c

ping_app = ping.c

all:
	@echo compiling lwip unix port
	@${CC} $(CFLAGS) -DLWIP_DEBUG -O0 -g \
		-I$(LWIPDIR)/../contrib/ports/unix/lib \
		-I$(LWIPDIR)/../contrib/ports/unix/port/include \
		-I$(LWIPDIR)/include \
		$(COREFILES) \
		$(lwipnetif_SRCS) \
		$(lwipcore4_SRCS) \
		$(lwipapi_SRCS) \
		$(LWIPUNIXPORT) \
		-pthread -fPIC -shared -Wl,-soname,libipxdp.so -o libipxdp.so
	@ls -l libipxdp.so
	@file libipxdp.so
	@echo compiling example app
	@${CC} $(CFLAGS) -O0 -g $(example_app) -I$(LWIPDIR)/../contrib/ports/unix/lib \
			     -I$(LWIPDIR)/../contrib/ports/unix/port/include \
			     -I$(LWIPDIR)/include  \
			     -I$(LWIPDIR)/../contrib \
			     -I$(LWIPDIR)/../contrib/apps/httpserver \
			     -L. -lipxdp -o http_server
	@ls -l http_server

	@echo compiling example ping
	@${CC} $(CFLAGS) -O0 -g $(ping_app) -I$(LWIPDIR)/../contrib/ports/unix/lib \
			     -I$(LWIPDIR)/../contrib/ports/unix/port/include \
			     -I$(LWIPDIR)/include  \
			     -I$(LWIPDIR)/../contrib \
			     -I$(LWIPDIR)/../contrib/apps/httpserver \
			     -L. -lipxdp -o ping

	@ls -l ping

check:
	LD_LIBRARY_PATH=. ./ping

clean:
	rm -rf libipxdp.so
	rm -rf http_server
	rm -rf ping 


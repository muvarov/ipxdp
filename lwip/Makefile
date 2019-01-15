LWIPDIR=/opt/Linaro/xdp/lwip.git/src
ARCH := x86
KSRC := /opt/Linaro/xdp/linux.git

include $(LWIPDIR)/Filelists.mk

LWIPUNIXPORT = $(LWIPDIR)/../contrib/ports/unix/port/perf.c \
		$(LWIPDIR)/../contrib/ports/unix/port/sys_arch.c \
		$(LWIPDIR)/../contrib/ports/unix/port/netif/sio.c \
		$(LWIPDIR)/../contrib/ports/unix/port/netif/fifo.c \
		$(LWIPDIR)/../contrib/ports/unix/port/netif/tapif.c \
		netif/af_xdp.c

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

AF_XDP_INCLUDE = -I$(KSRC)/tools/lib/ \
		-I$(KSRC)/tools/testing/selftests/bpf \
		-I$(KSRC)/tools/testing/selftests/bpf/include \
		-I$(KSRC)/tools/include -I. -I$(KSRC)/tools/lib/bpf \
		-I$(KSRC)/include/uapi

AF_XDP_LIBS = -lbpf -lelf

all:
	@echo compiling lwip unix port
	${CC} $(CFLAGS) -DLWIP_DEBUG -O0 -g \
		-I$(LWIPDIR)/../contrib/ports/unix/lib \
		-I$(LWIPDIR)/../contrib/ports/unix/port/include \
		-I$(LWIPDIR)/include \
		-Inetif \
		$(AF_XDP_INCLUDE) \
		$(COREFILES) \
		$(lwipnetif_SRCS) \
		$(lwipcore4_SRCS) \
		$(lwipapi_SRCS) \
		$(LWIPUNIXPORT) \
		$(AF_XDP_LIBS) \
		-pthread -fPIC -shared -Wl,-soname,libipxdp.so -o libipxdp.so
	@ls -l libipxdp.so
	@file libipxdp.so
	@echo compiling example app
	@${CC} $(CFLAGS) -O0 -g $(example_app) -I$(LWIPDIR)/../contrib/ports/unix/lib \
			     -I$(LWIPDIR)/../contrib/ports/unix/port/include \
			     -I$(LWIPDIR)/include  \
			     -I$(LWIPDIR)/../contrib \
			     -I$(LWIPDIR)/../contrib/apps/httpserver \
			     -I. \
			     -L. -lipxdp -o http_server
	@ls -l http_server

	@echo compiling example ping
	@${CC} $(CFLAGS) -O0 -g $(ping_app) -I$(LWIPDIR)/../contrib/ports/unix/lib \
			     -I$(LWIPDIR)/../contrib/ports/unix/port/include \
			     -I$(LWIPDIR)/include  \
			     -I$(LWIPDIR)/../contrib \
			     -I$(LWIPDIR)/../contrib/apps/httpserver \
			     -I. \
			     -L. -lipxdp -o ping

	@ls -l ping

check:
	echo LD_LIBRARY_PATH=.:$(KSRC)/tools/lib/bpf ./ping

clean:
	rm -rf libipxdp.so
	rm -rf http_server
	rm -rf ping 


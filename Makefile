ARCH ?= x86
KSRC ?= /opt/Linaro/xdp/linux.git

LLVM ?=
LLC=${LLVM}llc
CLANG=${LLVM}clang

CFLAGS = -Wno-unused-value -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end \
		-Wno-address-of-packed-member -Wno-tautological-compare \
		-Wno-unknown-warning-option -O2 -Wall
LDFLAGS = -lbpf -lelf -pthread

all: xdpsock lwip open62541 open62541-demos socket_wrapper iperf

.PHONY: iperf
iperf: lwip
	cd build_sources/iperf-xdp && \
	./configure CFLAGS="-I`pwd`/../../build_sources/lwip/src/include \
			    -I`pwd`/../../build_sources/lwip/contrib/ports/unix/lib \
			    -I`pwd`/../../build_sources/lwip/contrib/ports/unix/port/include" \
		    LDFLAGS="-L`pwd`/../../lwip -lipxdp"  && \
	make


.PHONY: socket_wrapper
socket_wrapper: open62541
	cd socket_wrapper && \
	LWIP_SRC=../build_sources/lwip/src \
	IPXDP=../lwip \
	make

.PHONY: xdpsock
xdpsock:
	$(CC) xdpsock_user.c -I./include -I$(KSRC)/tools/lib/ \
		-I$(KSRC)/tools/testing/selftests/bpf \
		-I$(KSRC)/tools/testing/selftests/bpf/include \
		-I$(KSRC)/tools/include -I. -I$(KSRC)/tools/lib/bpf \
		-I$(KSRC)/include/uapi -Wl,-Bstatic -L$(KSRC)/tools/lib/bpf/ \
		-Wl,-Bdynamic $(LDFLAGS) -o xdpsock -Wall

	${CLANG} -nostdinc \
		-I$(KSRC)/arch/$(ARCH)/include -I$(KSRC)/arch/$(ARCH)/include/generated \
		-I$(KSRC)/arch/$(ARCH)/include/generated/uapi -I$(KSRC)/include/uapi \
		-I$(KSRC)/samples/bpf \
		-I$(KSRC)/include -I$(KSRC)/arch/$(ARCH)/include/uapi \
		-I$(KSRC)/tools/testing/selftests/bpf/ \
		-D__KERNEL__ -D__BPF_TRACING__ \
		$(CFLAGS) \
		-O2 -emit-llvm -c xdpsock_kern.c

	${LLC} -march=bpf -filetype=obj xdpsock_kern.bc -o xdpsock_kern.o

.PHONY: lwip
lwip:
	cd lwip && ARCH=$(ARCH) KSRC=$(KSRC) LWIPDIR=../build_sources/lwip/src make

.PHONY: open62541
open62541:
	cd build_sources/open62541 && mkdir -p build && \
		sed -i 's/set(LWIP_SRC.*/set\(LWIP_SRC "..\/..\/..\/build_sources\/lwip\/src")/g' CMakeLists.txt && \
	cd build && cmake -DCMAKE_INSTALL_PREFIX=../../../build_install/open62541-install  ../ && \
	make && \
	rm -rf ../../../build_install/open62541-install && \
	make install

open62541-demos: open62541
	cd build_sources/open62541demos/open62541temp && \
	 LWIP_SRC=../../../build_sources/lwip/src \
	 IPXDP=../../../lwip \
	 OPCUA=../../../build_install/open62541-install \
	 OPCUA_SRC=../../../build_sources/open62541 \
	 make

.PHONY: clean
clean:
	rm -rf xdpsock
	rm -rf xdpsock_kern.o
	rm -rf xdpsock_kern.bc

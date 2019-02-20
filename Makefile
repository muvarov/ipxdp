ARCH ?= x86

TOPDIR = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
KSRC ?= $(TOPDIR)/build_sources/linux

LLVM ?=
LLC=${LLVM}llc
CLANG=${LLVM}clang

ifeq ($(ARCH),arm)
	export HOST=arm-linux-gnueabihf
	export CC=arm-linux-gnueabihf-gcc
	export LD=arm-linux-gnueabihf-ld
	export CROSS_COMPILE=arm-linux-gnueabihf-
endif

CFLAGS = -Wno-unused-value -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end \
		-Wno-address-of-packed-member -Wno-tautological-compare \
		-Wno-unknown-warning-option -O2 -Wall
LDFLAGS = -lbpf -lelf -pthread

all: xdpsock lwip open62541 open62541-demos socket_wrapper iperf


.PHONY: zlib
zlib:
	cd build_sources/zlib && \
		./configure && \
		make install prefix=`pwd`/../../build_install

.PHONY: elfutils
elfutils:
	cd build_sources/elfutils && \
	./configure --host=${HOST} --prefix=`pwd`/../../build_install \
		CFLAGS="-I`pwd`/../../build_install/include -Wno-implicit-fallthrough -Wno-error -O2"  \
		LDFLAGS="-L`pwd`/../../build_install/lib -lz" && \
	make install 

.PHONY: libbpf
libbpf: zlib elfutils
	cd build_sources/linux && \
	make multi_v7_defconfig && \
		cd tools/lib/bpf && \
		make CFLAGS="-I`pwd`/../../../../../build_install/include"  && \
		cp libbpf.so ../../../../../build_install/lib/

.PHONY: iperf
iperf: lwip
	cd build_sources/iperf-xdp && \
	./configure --host=${HOST} CFLAGS="-I`pwd`/../../build_sources/lwip/src/include \
			    -I`pwd`/../../build_sources/lwip/contrib/ports/unix/lib \
			    -I`pwd`/../../build_sources/lwip/contrib/ports/unix/port/include \
			    -I`pwd`/../../lwip \
	 		    -I$(TOPDIR)/build_install/include -L$(TOPDIR)/build_install/lib -lbpf -lelf -lz -lipxdp" \
		    LDFLAGS="-L`pwd`/../../lwip -lipxdp" \
		--prefix=`pwd`/../../build_install --without-openssl && \
	make && \
	make install

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
		-I$(TOPDIR)/build_install/include -L$(TOPDIR)/build_install/lib \
		-I$(KSRC)/include/uapi -Wl,-Bstatic -L$(KSRC)/tools/lib/bpf/ \
		-Wl,-Bdynamic $(LDFLAGS) -lz -o xdpsock -Wall

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
	cd lwip && \
	ARCH=$(ARCH) KSRC=$(KSRC) LWIPDIR=../build_sources/lwip/src \
		EXT_FLAGS="-I$(TOPDIR)/build_install/include -L$(TOPDIR)/build_install/lib"\
		make

.PHONY: open62541
open62541:
	cd build_sources/open62541 && mkdir -p build && \
		sed -i 's/set(LWIP_SRC.*/set\(LWIP_SRC "..\/..\/..\/build_sources\/lwip\/src")/g' CMakeLists.txt && \
	cd build && cmake -DCMAKE_INSTALL_PREFIX=../../../build_install ../ && \
	make && \
	make install

open62541-demos: open62541
	cd build_sources/open62541demos/open62541temp && \
	 LWIP_SRC=../../../build_sources/lwip/src \
	 IPXDP=../../../lwip \
	 OPCUA=../../../build_install \
	 OPCUA_SRC=../../../build_sources/open62541 \
	 CFLAGS="-I$(TOPDIR)/build_install/include -L$(TOPDIR)/build_install/lib -lbpf -lelf -lz -lipxdp" \
	 make

.PHONY: install
install: all
	cp ./lwip/libipxdp.so ./build_install/lib/
	cp xdpsock_kern.o ./build_install/bin/lwip_af_xdp_kern.o

.PHONY: clean
clean:
	rm -rf xdpsock
	rm -rf xdpsock_kern.o
	rm -rf xdpsock_kern.bc

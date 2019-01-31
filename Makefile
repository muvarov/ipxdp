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

all: xdpsock

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

.PHONY: clean
clean:
	rm -rf xdpsock
	rm -rf xdpsock_kern.o
	rm -rf xdpsock_kern.bc

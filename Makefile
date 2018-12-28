
KSRC := /opt/Linaro/xdp/linux.git

LLVM := /opt/Linaro/xdp/llvm/build/bin
LLC=${LLVM}/llc
CLANG=${LLVM}/clang

all:
	${CC} xdpsock_user.c -I./include -I${KSRC}/tools/lib/ -I${KSRC}/tools/testing/selftests/bpf -I${KSRC}/tools/testing/selftests/bpf/include \
	 -I${KSRC}/tools/include -I. -I${KSRC}/tools/lib/bpf -I${KSRC}/include/uapi \
	 -Wl,-Bstatic -L${KSRC}/tools/lib/bpf -lbpf -Wl,-Bdynamic -lelf -pthread -o xdpsock -Wall

	${CLANG}  -nostdinc -isystem /usr/lib/gcc/x86_64-linux-gnu/5/include -I${KSRC}/arch/x86/include -I${KSRC}/arch/x86/include/generated  -I${KSRC}/include \
	 -I${KSRC}/arch/x86/include/uapi -I${KSRC}/arch/x86/include/generated/uapi -I${KSRC}/include/uapi -I${KSRC}/include/generated/uapi \
	 -I${KSRC}/samples/bpf \
	 -I${KSRC}/tools/testing/selftests/bpf/ \
	 -D__KERNEL__ -D__BPF_TRACING__ -Wno-unused-value -Wno-pointer-sign \
	 -D__TARGET_ARCH_x86 -Wno-compare-distinct-pointer-types \
	 -Wno-gnu-variable-sized-type-not-at-end \
	 -Wno-address-of-packed-member -Wno-tautological-compare \
	 -Wno-unknown-warning-option  \
	 -O2 -emit-llvm -c xdpsock_kern.c
	${LLC} -march=bpf -filetype=obj xdpsock_kern.bc -o xdpsock_kern.o

clean:
	rm -rf xdpsock
	rm -rf xdpsock_kern.o
	rm -rf xdpsock_kern.bc

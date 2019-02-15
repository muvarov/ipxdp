Requirements:
	- linux kernel with AF_XDP support
	- llvm with bpf target.
		$ git clone http://llvm.org/git/llvm.git
		$ cd llvm/tools
		$ git clone --depth 1 http://llvm.org/git/clang.git
		$ cd ..; mkdir build; cd build
		$ cmake .. -DLLVM_TARGETS_TO_BUILD="BPF;X86"
		$ make -j $(getconf _NPROCESSORS_ONLN)
		$ cd build/bin; export PATH=`pwd`:$PATH
	- libbpf.so from kernel sources:
		make samples/bpf/ LLC=~/git/llvm/build/bin/llc CLANG=~/git/llvm/build/bin/clang
	(more details here: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/samples/bpf/README.rst)

Build:
- git clone https://github.com/muvarov/ipxdp.git
- cd ipxdp
- repo init -u https://github.com/muvarov/ipxdp.git
- repo sync
- make install

Run:
cd  build_install/lib
export LD_LIBRARY_PATH=`pwd`

On XDP box:
enp8s0 - is my testing interface
ifconfig  enp8s0 0  (disable IP address)
ifconfig  enp8s0 promisc (setup promisc mode)
./iperf3 -s (run server)

On remote box with original not modified iperf3:
./iperf3 -c 192.168.1.200 -t 600


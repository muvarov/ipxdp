LWIP_FLAGS = "-I${LWIP_SRC}/include -I${LWIP_SRC}/../contrib/ports/unix/lib \
	-I${LWIP_SRC}/../contrib/ports/unix/port/include \
	-I/opt/Linaro/xdp/open62541.git/arch/posix"

all:
	${CC} -O0 -g -Dsocket_wrapper_EXPORTS -I. -fPIC -std=gnu99 -Wall -Wshadow -Wmissing-prototypes -Wcast-align -Werror=address \
 		-Wstrict-prototypes -Werror=strict-prototypes -Wwrite-strings -Werror=write-strings -Werror-implicit-function-declaration \
		-Wpointer-arith -Werror=pointer-arith -Wdeclaration-after-statement -Werror=declaration-after-statement -Wreturn-type \
		-Werror=return-type -Wuninitialized -Werror=uninitialized -Werror=strict-overflow -Wstrict-overflow=2 -Wno-format-zero-length \
		-Wformat -Wformat-security -Werror=format-security -fno-common -fstack-protector-strong -D_GNU_SOURCE \
		-I${LWIP_SRC}/include  -I${LWIP_SRC}/../contrib/ports/unix/lib \
		-I${LWIP_SRC}/../contrib/ports/unix/port/include \
		${LWIP_FLAGS} \
		-o socket_wrapper.o -c socket_wrapper.c

	${CC} -O0 -g -Dsocket_wrapper_EXPORTS -I. -fPIC -std=gnu99 -Wall -Wshadow -Wmissing-prototypes -Wcast-align -Werror=address \
 		-Wstrict-prototypes -Werror=strict-prototypes -Wwrite-strings -Werror=write-strings -Werror-implicit-function-declaration \
		-Wpointer-arith -Werror=pointer-arith -Wdeclaration-after-statement -Werror=declaration-after-statement -Wreturn-type \
		-Werror=return-type -Wuninitialized -Werror=uninitialized -Werror=strict-overflow -Wstrict-overflow=2 -Wno-format-zero-length \
		-Wformat -Wformat-security -Werror=format-security -fno-common -fstack-protector-strong -D_GNU_SOURCE \
		-I${LWIP_SRC}/include  -I${LWIP_SRC}/../contrib/ports/unix/lib \
		-I${LWIP_SRC}/../contrib/ports/unix/port/include \
		${LWIP_FLAGS} \
		-I${IPXDP} \
		-o lwip_init.o -c lwip_init.c


	${CC} -fPIC  -shared -Wl,-soname,libsocket_wrapper.so -o libsocket_wrapper.so socket_wrapper.o lwip_init.o -ldl -lpthread -L${IPXDP} -lipxdp 

clean:
	rm -rf libsocket_wrapper.so
	rm -rf socket_wrapper.o


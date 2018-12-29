all:
	${CC} -Dsocket_wrapper_EXPORTS -I. -fPIC -std=gnu99 -Wall -Wshadow -Wmissing-prototypes -Wcast-align -Werror=address -Wstrict-prototypes -Werror=strict-prototypes -Wwrite-strings -Werror=write-strings -Werror-implicit-function-declaration -Wpointer-arith -Werror=pointer-arith -Wdeclaration-after-statement -Werror=declaration-after-statement -Wreturn-type -Werror=return-type -Wuninitialized -Werror=uninitialized -Werror=strict-overflow -Wstrict-overflow=2 -Wno-format-zero-length -Wformat -Wformat-security -Werror=format-security -fno-common -fstack-protector-strong -D_GNU_SOURCE -o socket_wrapper.o -c socket_wrapper.c

	${CC} -fPIC  -shared -Wl,-soname,libsocket_wrapper.so -o libsocket_wrapper.so socket_wrapper.o -ldl -lpthread 

clean:
	rm -rf libsocket_wrapper.so
	rm -rf socket_wrapper.o


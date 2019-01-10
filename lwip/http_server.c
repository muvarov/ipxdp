#include <unistd.h>

extern void http_server_netconn_init(void);

int main(void)
{

	http_server_netconn_init();

	while(1)
		sleep(100);
	return 0;
}


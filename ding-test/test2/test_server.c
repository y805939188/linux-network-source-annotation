#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
int main()
{
	int ret = 0;
	int fd = socket(45, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("socket() failed,%s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_in addr = { .sin_family = 45,
				    .sin_port = htons(3190),
				    .sin_addr.s_addr = htonl(INADDR_ANY) };
	bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	char buffer[256];
	while (1) {
		bzero(buffer, sizeof(buffer));
		struct sockaddr_in addr_client;
		socklen_t addr_len = sizeof(addr_client);
		ret = recvfrom(
			fd, buffer, sizeof(buffer), 0,
			(struct sockaddr *)&addr_client, &addr_len
		);
		if (strlen(buffer) != 0) {
			printf("从 client 端接收到: %s\n", buffer);
		}
		// printf("从 client 端接收到: %s\n", buffer);
		// fprintf(stderr, "从 client 端接收到: %s\n", buffer);
		// printf("接收到消息");
		sleep(1);
	}
	close(fd);
	return 0;
}

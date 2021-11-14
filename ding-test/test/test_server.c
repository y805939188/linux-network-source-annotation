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
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("socket() failed,%s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_in addr = { .sin_family = AF_INET,
				    .sin_port = htons(3190),
				    .sin_addr.s_addr = htonl(INADDR_ANY) };
	bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	while (1) {
		char buffer[256];
		struct sockaddr_in addr_client;
		socklen_t addr_len = sizeof(addr_client);
    // char *msg = "即将接收消息\n";
    // write(STDOUT_FILENO, msg, strlen(msg));
    printf("正在接收消息: %x,%d\n", ntohl(buffer[0]), ntohl(buffer[1]));
		ret = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr_client, &addr_len);
		// printf("从 client 端接收到: %x,%d\n", ntohl(buf[0]), ntohl(buf[1]));
    printf("从 client 端接收到: %x,%d\n", ntohl(buffer[0]), ntohl(buffer[1]));
    char *msg2 = "消息已接收\n";
    // write(STDOUT_FILENO, msg2, strlen(msg2));
		sleep(1);
	}
	close(fd);
	return 0;
}

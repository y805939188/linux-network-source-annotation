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
	int fd = socket(45, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("socket() failed,%s\n", strerror(errno));
		return -1;
	}
	struct sockaddr_in addr = { .sin_family = 45,
				    .sin_port = htons(3190),
				    .sin_addr.s_addr = htonl(INADDR_ANY) };
  struct sockaddr *_addr = (struct sockaddr *)&addr;
  int count = 1;
  char buffer[256];
	while (1) {
    bzero(buffer, sizeof(buffer));
    count++;
	 	sprintf(buffer,"ding-%d\n", count);
    // printf("这里要发送的数据是: %s", buffer);
		fprintf(stderr, "这里要发送的数据是: %s\n", buffer);
		sendto(fd, buffer, sizeof(buffer), 0, _addr, sizeof(addr));
		sleep(3);
	}
	close(fd);
	return 0;
}

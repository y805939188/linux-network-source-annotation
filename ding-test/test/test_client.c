#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
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
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("socket 执行失败, 没有文件描述符: %s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_in addr = {
		.sin_family = AF_INET, // 自定义的协议簇
		.sin_port = htons(3190), // 端口号
		.sin_addr.s_addr = htonl(INADDR_ANY) // 0.0.0.0
	};
	struct sockaddr *_addr = (struct sockaddr *)&addr;

	char buffer[256];
	while (1) {
		bzero(buffer, sizeof(buffer));
		//从标准输入设备取得字符串
		int len = read(STDIN_FILENO, buffer, sizeof(buffer));
		char *prompt = "3 秒后将发送数据\n";
		write(STDOUT_FILENO, prompt, strlen(prompt));
		sleep(3);
		sendto(fd, buffer, sizeof(buffer), 0, _addr, sizeof(addr));
    char *msg = "消息已发送\n";
    write(STDOUT_FILENO, msg, strlen(msg));
	}
	close(fd);
	return 0;
}

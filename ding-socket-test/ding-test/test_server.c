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
    if(fd < 0)
    {
        printf("socket() failed,%s\n",strerror(errno));
        return -1;
    }
    
    struct sockaddr_in addr =
    {
        .sin_family     = 45,
        .sin_port     = htons(3355),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    while(1)
    {
        uint32_t buf[2] = {0};
        
        struct sockaddr_in addr_client;
        socklen_t addr_len = sizeof(addr_client);
        ret = recvfrom(fd, buf, sizeof(buf), 0, 
            (struct sockaddr *)&addr_client, &addr_len);
        printf("-------------------> USER SPACE RECV: %x,%d\n", ntohl(buf[0]), ntohl(buf[1]));
        sleep(1);
    }
    close(fd);
    return 0;
}


#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define HELLO_PORT 12345
#define HELLO_GROUP "255.255.255.255"
//#define HELLO_GROUP "192.168.1.255"
//#define HELLO_GROUP "224.26.91.1"

int main(int argc, char *argv[])
{
	struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(HELLO_GROUP);
    saddr.sin_port = htons(HELLO_PORT);

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return 1;
	}

	if (bind(fd, (sockaddr*)&saddr, sizeof(saddr)) < 0) {
		perror("bind");
		return 1;
	}

	for (;;) {
		socklen_t slen = sizeof(saddr);
		char buf[80];
		ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (sockaddr*)&saddr, &slen);
		if (n < 0) {
			perror("recvfrom");
			break;
		}
		buf[n] = 0;
		printf("message: '%s'\n", buf);
	}

	close(fd);

	return 0;
}

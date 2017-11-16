#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
const char *HOST = "fa17-cs241-185.cs.illinois.edu";
const char *PORT = "16354";

int main(int argc, char **argv)
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror(NULL);
        exit(1);
    }


    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    /*QUESTION 6*/
    struct addrinfo *result;
    int err = getaddrinfo(HOST, PORT, &hints, &result);
    if (err) {
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(1);
    }

    /*QUESTION 7*/
    if(connect(sock_fd, result->ai_addr, result->ai_addrlen)){
        perror(NULL);
        exit(1);
    }

	char *buffer = "honor.fs";
	printf("Syncing: %s\n", buffer);
	printf("===\n");

	write(sock_fd, buffer, strlen(buffer));

    char *input;
    size_t len;
    ssize_t read;
	printf("Please input file content:\n");
    read = getline(&input, &len, stdin);
	printf("Sending content: %s", input);
	printf("===\n");

	write(sock_fd, input, read);
    free(input);

    if (shutdown(sock_fd, SHUT_RDWR)) {
        perror(NULL);
        exit(1);
    }
    close(sock_fd);

    freeaddrinfo(result);
    return 0;
}


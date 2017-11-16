#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
const char *PORT = "16354";

int main(int argc, char **argv)
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror(NULL);
        exit(1);
    }

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *result;
    int err = getaddrinfo(NULL, PORT, &hints, &result);
    if (err) {
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(1);
    }

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen)) {
        perror(NULL);
        exit(1);
    }

    if (listen(sock_fd, 10) != 0) {
        perror(NULL);
        exit(1);
    }
    
    printf("Waiting for connection...\n");
    int client_fd = accept(sock_fd, NULL, NULL);
    if (client_fd == -1) {
        perror(NULL);
        exit(1);
    }
    printf("Connection made: client_fd=%d\n", client_fd);

    char buffer[1000];
    int len = read(client_fd, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';

    printf("Read %d chars\n", len);
    printf("===\n");
    printf("%s\n", buffer);

    if (shutdown(sock_fd, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(sock_fd);
    if (shutdown(client_fd, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(client_fd);

    freeaddrinfo(result);
    return 0;
}

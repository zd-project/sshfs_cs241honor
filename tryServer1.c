#include<stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>
#include <netdb.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
int main(int argc, char* argv[]) {
        int ret_code;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    ret_code = bind(sock_fd, (struct sockaddr*)&server, sizeof(server));
    if (ret_code < 0) {
        perror("bind failed\n");
        exit(1);
    }

    listen(sock_fd, 10);


    // accept and incoming connection
    puts("Waiting for incoming connections");
    //c = sizeof(struct sockaddr_in);

    // accept connection from an incoming client
        struct sockaddr_in client;
        int c = sizeof(struct sockaddr_in);
    int client_sock = accept(sock_fd, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }
    printf("Connection accepted\n");

    char client_message[2000];
    int read_size;
    // receive a message from client
    while ((read_size = read(client_sock, client_message, sizeof(client_message))) > 0) {
        // send the message back to client
        client_message[read_size] = '\0';
        write(client_sock, client_message, strlen(client_message));
    }

    return 0;
}
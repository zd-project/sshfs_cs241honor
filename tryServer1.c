#include<stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>
#include <netdb.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto_client_master.h"
// handle message sent by client
void receive(char client_message[100*K], int client_sock);
// send feedback to client
void write_back(char* message, int client_sock);
int main(int argc, char* argv[]) {
    int ret_code;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9000);

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

    char* client_message = calloc(100*K, 1);
    int read_size;
    // receive a message from client
    while ((read_size = read(client_sock, client_message, sizeof(client_message))) > 0) {
        receive(client_message, client_sock);
    }

    return 0;
}

void receive(char client_message[100*K], int client_sock) {
    META_DATA* meta = (META_DATA*) client_message;
    unsigned data_length = meta -> data_length;
    
    printf("data_length: %u\n", data_length);
    printf("func code: %d\n", meta -> func_code);
    if (meta -> func_code == FUNC_FILE_UPLOAD){ 
        FILE_TRANSMIT* file_block = (FILE_TRANSMIT*) client_message;
        char* file_name = file_block -> file_name;
        printf("file_name: %s\n", file_name);   
        char* buffer = file_block -> buffer;
        printf("filecontent: %s\n", buffer);
        FILE* fp = fopen(file_name, "w+");
        fwrite((void*)buffer, data_length - 32, 1, fp);
        fclose(fp);
        write_back("File upload success", client_sock);
    }
    else if(meta -> func_code == FUNC_FILE_DOWNLOAD) {
        FILE_TRANSMIT* file_block = (FILE_TRANSMIT*) client_message;
        FILE_TRANSMIT* file_to_upload = calloc(1,sizeof(FILE_TRANSMIT));
        char* file_name_requested = file_block -> file_name;
        // get the file and its size
        FILE* fp = fopen(file_name_requested, "r");
        if (!fp) {
            write_back("File not exist.", client_sock);
            return;
        }
        fseek(fp, 0L, SEEK_END);
        long file_size = ftell(fp) - 1;
        rewind(fp);

        file_to_upload -> meta_data.data_length = 32 + file_size;
        file_to_upload -> meta_data.func_code = FUNC_FILE_UPLOAD;

        strcpy(file_to_upload -> file_name, file_name_requested);
        fread(file_to_upload -> buffer, sizeof(char), file_size, fp);
        write(client_sock, file_to_upload, sizeof(FILE_TRANSMIT));
        fclose(fp);
    }
    else if(meta -> func_code == FUNC_EXECUTE_CMD) {
        EXECUTE_CMD* cmd_block = (EXECUTE_CMD*) client_message;
        char* cmd = cmd_block -> command;
        int fd[2];
        pipe(fd);

        int child = fork();
        if (child) {
            // parent
            close(fd[1]);
            int status;
            waitpid(child, &status, 0);
            char buffer[16*K];
            int bytesread = read(fd[0], buffer, sizeof(buffer));
            write_back(buffer, client_sock);
            close(fd[0]);
        }
        else {
            // child
            close(fd[0]);
            dup2(fd[1], 1);
            int ret = system(cmd);
            if (ret != 0) {
                printf("fail to run command %s\n", cmd);
            }
            close(fd[1]);
        }
    }
}

void write_back(char* message, int client_sock) {
    FEEDBACK* feedback_block = calloc(1,sizeof(FEEDBACK));
    feedback_block -> meta_data.func_code = FUNC_FEEDBACK;
    feedback_block -> meta_data.data_length = strlen(message);
    strcpy(feedback_block -> feedback, message);
    write(client_sock, feedback_block, sizeof(FEEDBACK));
}
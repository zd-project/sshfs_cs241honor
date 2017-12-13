#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
#include <netdb.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto_client_master.h"
#include "server_utils.h"

#define PORT_NUM "10086"
#define CLIENT_MESSAGE_SIZE 100*K

// handle message sent by client
void receive(char client_message[100*K], int client_sock);
// send feedback to client
void write_back(char* message, int client_sock);
// action functions according to func code
void upload_file(char client_message[CLIENT_MESSAGE_SIZE], unsigned data_length, int client_sock);
void download_file(char client_message[CLIENT_MESSAGE_SIZE], unsigned data_length, int client_sock);
void exec_cmd(char client_message[CLIENT_MESSAGE_SIZE], int client_sock);

int main(int argc, char* argv[]) {
    int ret_code;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct addrinfo* res;
    struct addrinfo hint;
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

    if ((ret_code = getaddrinfo(NULL, PORT_NUM, &hint, &res)) != 0) {
        fprintf(stderr, "%s\n", gai_strerror(ret_code));
        exit(1);
    }

    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    ret_code = bind(server_socket, res -> ai_addr, res -> ai_addrlen);
    if (ret_code < 0) {
        perror(NULL);
        exit(1);
    }

    ret_code = listen(server_socket, 10);
    if (ret_code != 0) {
        perror(NULL);
        exit(1);
    }

    fprintf(stderr, "hi\n");
    fprintf(stderr, "Waiting for incoming connections");

    // accept connection from an incoming client
    int client_sock = accept(server_socket, NULL, NULL);
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }
    fprintf(stderr, "Connection accepted\n");
    char client_message[CLIENT_MESSAGE_SIZE];
    int read_size;

    // receive messages from client
    while (1) {
        read_size = (int)read_all_from_socket(client_sock, client_message, sizeof(client_message));
        if (read_size <= 0) {
            sleep(1);
        }
        else {
            receive(client_message, client_sock);
        }
    }
    return 0;
}

void receive(char client_message[CLIENT_MESSAGE_SIZE], int client_sock) {
    fprintf(stderr, "enter receive\n"); 
    META_DATA* meta = (META_DATA*) client_message;
    unsigned data_length = meta -> data_length;
    // printf("data_length: %u\n", data_length);
    // printf("func code: %d\n", meta -> func_code);

    if (meta -> func_code == FUNC_FILE_UPLOAD){
        upload_file(client_message, data_length, client_sock);
    }
    else if(meta -> func_code == FUNC_FILE_DOWNLOAD) {
        download_file(client_message, data_length, client_sock);
    }
    else if(meta -> func_code == FUNC_EXECUTE_CMD) {
        exec_cmd(client_message, client_sock);
    }
}

void exec_cmd(char client_message[CLIENT_MESSAGE_SIZE], int client_sock) {
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
        read(fd[0], buffer, sizeof(buffer));
        write_back(buffer, client_sock);
        close(fd[0]);
    }
    else {
        // child
        close(fd[0]);
        // redirect stdout to pipe
        dup2(fd[1], 1);
        int ret = system(cmd);
        if (ret != 0) {
            printf("fail to run command %s\n", cmd);
        }
        close(fd[1]);
        exit(0);
    }
}

void upload_file(char client_message[CLIENT_MESSAGE_SIZE] , unsigned data_length, int client_sock) {
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

void download_file(char client_message[CLIENT_MESSAGE_SIZE], unsigned data_length, int client_sock) {
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
    long file_size = ftell(fp);
    rewind(fp);

    file_to_upload -> meta_data.data_length = 32 + file_size;
    file_to_upload -> meta_data.func_code = FUNC_FILE_UPLOAD;

    strcpy(file_to_upload -> file_name, file_name_requested);
    fread(file_to_upload -> buffer, sizeof(char), file_size, fp);
    write(client_sock, file_to_upload, sizeof(FILE_TRANSMIT));
    fclose(fp);
    free(file_to_upload);
}

void write_back(char* message, int client_sock) {
    printf("writeback\n");
    if (message[strlen(message)-1] == '\n') {
        message[strlen(message)-1] = '\0';
    }
    FEEDBACK* feedback_block = calloc(1,sizeof(FEEDBACK));
    feedback_block -> meta_data.func_code = FUNC_FEEDBACK;
    feedback_block -> meta_data.data_length = strlen(message);
    strcpy(feedback_block -> feedback, message);
    ssize_t write_byte =  write(client_sock, feedback_block, sizeof(FEEDBACK));
    printf("write %zd\n", write_byte);
    free(feedback_block);
    feedback_block = NULL;
}


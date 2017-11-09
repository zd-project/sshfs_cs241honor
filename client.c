#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "proto_client_master.h"

#define DEAN_VM_IP "172.22.148.84"
#define DEAN_PC_IP "10.192.171.18"

int main(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		printf("sock failed\n");
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(DEAN_VM_IP);
	server.sin_port = htons(9000);

	printf("start connect\n");

	int retCode = connect(sock, (const struct sockaddr*) &server, sizeof(server));
	if(retCode < 0){
		perror("connect fail");
		return 0;
	}

	printf("connect done\n");

	// get the file and its size
	FILE* f = fopen("test.txt", "r");
	fseek(f, 0L, SEEK_END);
	long file_size = ftell(f) - 1;
	rewind(f);

	// get the data ready
	FILE_TRANSMIT* data_sent = calloc(sizeof(FILE_TRANSMIT), 0);
	data_sent->meta_data.data_length = sizeof(META_DATA) + file_size;
	data_sent->meta_data.func_code = 1;
	strcpy(data_sent->file_name, "test.txt");
	fread(data_sent->buffer, sizeof(char), file_size, f);

	// send the data accross
	retCode = send(sock, data_sent, sizeof(FILE_TRANSMIT), 0);
	if(retCode < 0){
		printf("send failed\n");
		return 0;
	}

	printf("send done\n");

	// receive a confirmation
	char* buf_recv = calloc(128, sizeof(char));
	retCode = recv(sock, buf_recv, 128, 0);
	if(retCode < 0){
		printf("recv failed\n");
		return 0;
	}
	printf("%s\n", buf_recv);

	printf("recv done\n");

	// char* buf = malloc(128);
	// char* buf_recv = calloc(128, sizeof(char));
	// strcpy(buf, "Hello World");
	// while(1){
	// 	sleep(1);
	// 	retCode = send(sock, buf, strlen(buf), 0);
	// 	if(retCode < 0){
	// 		printf("send failed");
	// 		return 0;
	// 	}

	// 	printf("send done\n");

	// 	retCode = recv(sock, buf_recv, 128, 0);
	// 	if(retCode < 0){
	// 		printf("recv failed");
	// 		return 0;
	// 	}
	// 	printf("server: %s\n", buf_recv);
	// }

	close(sock);
	return 0;
}
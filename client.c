#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int main(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		printf("sock failed\n");
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("172.22.148.84");
	server.sin_port = htons(8888);

	int retCode = connect(sock, (const struct sockaddr*) &server, sizeof(server));
	if(retCode < 0){
		perror("connect fail");
		return 0;
	}

	printf("connect done\n");

	char* buf = malloc(128);
	char* buf_recv = calloc(128, sizeof(char));
	strcpy(buf, "Hello World");
	while(1){
		sleep(1);
		retCode = send(sock, buf, strlen(buf), 0);
		if(retCode < 0){
			printf("send failed");
			return 0;
		}

		printf("send done\n");

		retCode = recv(sock, buf_recv, 128, 0);
		if(retCode < 0){
			printf("recv failed");
			return 0;
		}
		printf("server: %s\n", buf_recv);
	}

	close(sock);
	return 0;
}
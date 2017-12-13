#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fs_server.h"
#include "filemgr.h"
#include "protocol.h"
#include "types.h"

Netend master;
Netend slaves[SLAVE_MAX];
Slaveid slave_cnt = 0;
pthread_mutex_t slave_mtx;

int server_init () {
	int error = 0;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;  // allow multiple connections
	
	error = getaddrinfo(NULL, "9001", &hints, &res);
		if (error) {
			printf("Failed getaddrinfo(): %s\n", gai_strerror(error));
			return -1;
		}
	master.fd = socket(res->ai_family, res->ai_socktype, 0);
		if (master.fd == -1) {
			printf("Failed socket()\n");
			return -1;
		}
	error = bind(master.fd, res->ai_addr, res->ai_addrlen);
		if (error == -1) {
			printf("Failed bind()\n");
			return -1;
		}
	listen(master.fd, 16);
		if (error == -1) {
			printf("Failed listen()\n");
			return -1;
		}
	
	memcpy(&(master.info), (struct sockaddr_in *)res->ai_addr, sizeof(struct sockaddr_in));
	char* ip = inet_ntoa(master.info.sin_addr);
	strcpy(master.ip, ip);
	master.port = ntohs(master.info.sin_port);
	master.active = true;

	return 0;
}

void *slave_accept (void *args) {
	pthread_mutex_init(&slave_mtx, NULL);
	while (slave_cnt < SLAVE_MAX) {
		// accept a slave
		Netend slave;
		socklen_t addrlen = sizeof(struct sockaddr);
		slave.fd = accept(master.fd, (struct sockaddr *)&(slave.info), &addrlen);
			if (slave.fd == -1) {
				printf("Failed accept(): %d\n", errno);
				continue;
			}
        /*fprintf(stderr, "accpeted\n");*/
		char* ip = inet_ntoa(slave.info.sin_addr);
		strcpy(slave.ip, ip);
		slave.port = ntohs(slave.info.sin_port);
		slave.active = true;
		
		// write slave info into array
		pthread_mutex_lock(&slave_mtx);
		memcpy(&slaves[slave_cnt], &slave, sizeof(Netend));
        /*fprintf(stderr, "start filemgr init slave\n");*/
		Filemgr_init_slave(slave_cnt, slave.fd);
        /*fprintf(stderr, "finish filemgr init slave\n");*/
		slave_cnt ++;
		pthread_mutex_unlock(&slave_mtx);
        /*fprintf(stderr, "one iteration\n");*/
	}
    return NULL;
}

void slave_delete (int slave_fd) {
	pthread_mutex_lock(&slave_mtx);
	for (int index = 0; index < slave_cnt; index ++) {
		if (slaves[index].fd == slave_fd) {
			slaves[index].active = false;
		}
	}
	pthread_mutex_unlock(&slave_mtx);
}

int server_run () {
	int error = 0;
	
	error = server_init();
    /*fprintf(stderr, "finish init\n");*/
		if (error) {
			printf("Failed server_init()\n");
			return -1;
		}
	pthread_t thread_slave_accept;
	error = pthread_create(&thread_slave_accept, NULL, slave_accept, NULL);
		if (error) {
			printf("Failed spawn slave_accept()\n");
			return -1;
		}

	return 0;
}


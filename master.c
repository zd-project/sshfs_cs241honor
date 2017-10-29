#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include "master.h"
#include "protocol_master_slave.h"

Netend master;
Netend slaves[16];
int slave_cnt = 0;
pthread_mutex_t slave_mutex;
pthread_cond_t slave_cv;
pthread_mutex_t slave_cvmtx;

// Initialize master server
int server_init () {
	int error = 0;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;  // allow multiple connections
	
	error = getaddrinfo(NULL, "18004", &hints, &res);
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
	master.ip = inet_ntoa(master.info.sin_addr);
	master.port = ntohs(master.info.sin_port);

	return 0;
}

// Accept slave connections continuously
void *slave_accept (void *args) {
	pthread_mutex_init(&slave_mutex, NULL);
	pthread_cond_init(&slave_cv, NULL);
	pthread_mutex_init(&slave_cvmtx, NULL);
	while (1) {
		// pause accepting when slaves array is full
		pthread_mutex_lock(&slave_cvmtx);
		while (slave_cnt == sizeof(slaves)/sizeof(Netend)) {
			pthread_cond_wait(&slave_cv, &slave_cvmtx);
		}
		pthread_mutex_unlock(&slave_cvmtx);

		// accept a slave
		Netend slave;
		slave.fd = accept(master.fd, (struct sockaddr *)&(slave.info), NULL);
			if (slave.fd == -1) {
				printf("Failed accept()\n");
				continue;
			}
		slave.ip = inet_ntoa(slave.info.sin_addr);
		slave.port = ntohs(slave.info.sin_port);
		slave.busy = false;
		
		// write slave info into array
		pthread_mutex_lock(&slave_mutex);
		memcpy(&slaves[slave_cnt], &slave, sizeof(Netend));
		slave_cnt ++;
		pthread_mutex_unlock(&slave_mutex);
	}
}

// Assign task to an idle slave
void *assign_task (void *args) {
	Message *input = (Message *)args;
	pthread_mutex_lock(&slave_mutex);
	int index;
	for (index = 0; index < slave_cnt; index ++) {
		if (slaves[index].busy == false) {
			slaves[index].busy = true;
			pthread_mutex_unlock(&slave_mutex);
			write(slaves[index].fd, input, sizeof(input->len) + input->len);
			break;
		}
	}
	Message *output = (Message *)malloc(sizeof(Message));
	read(slaves[index].fd, &(output->len), sizeof(output->len));
	read(slaves[index].fd, &(output->buf), output->len);
	pthread_mutex_lock(&slave_mutex);
	slaves[index].busy = false;
	pthread_mutex_unlock(&slave_mutex);
	return output;
}

// Delete a disconnected slave
void slave_delete (int slave_fd) {
	pthread_mutex_lock(&slave_mutex);
	for (int index = 0; index < slave_cnt; index ++) {
		if (slaves[index].fd == slave_fd) {
			// remove the slave at slaves[index]
			if (index != slave_cnt - 1) {
				// move the last slave into the empty slot
				memcpy(&slaves[index], &slaves[slave_cnt - 1], sizeof(Netend));
			}
			slave_cnt --;
			pthread_cond_signal(&slave_cv);
			break;
		}
	}
	pthread_mutex_unlock(&slave_mutex);
}

int main (int argc, char **argv) {
	int error = 0;
	
	error = server_init();
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
	
	while (slave_cnt == 0);
	printf("Slave connected\n");
	MessageInput input;
	input.len = 6;
	input.func_code = 1;
	input.buf[0] = 'l';
	input.buf[1] = 's';
	Message *output = (Message *)assign_task(&input);
	printf("%s\n", output->buf);
	
	pthread_join(thread_slave_accept, NULL);

	return 0;
}


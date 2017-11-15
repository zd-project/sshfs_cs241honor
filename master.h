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

typedef struct {
	int fd;
	struct sockaddr_in info;
	char *ip;
	unsigned short port;
	bool busy;
} Netend;

int server_init ();
void *slave_accept (void *args);
void *assign_task (void *args);
void slave_delete (int slave_fd);


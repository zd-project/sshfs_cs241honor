#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "types.h"

typedef struct {
	int fd;
	struct sockaddr_in info;
	char *ip;
	unsigned short port;
	bool active;
} Netend;

extern Netend master;
extern Netend slaves[SLAVE_MAX];

int server_init ();
void *slave_accept (void *args);
void slave_delete (int slave_fd);
Slaveid get_target_slave ();


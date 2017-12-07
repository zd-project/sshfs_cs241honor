#pragma once

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
	char ip[256];
	unsigned short port;
	bool active;
} Netend;

extern Netend master;
extern Netend slaves[SLAVE_MAX];

// Initialize master server
int server_init ();
// Accept slave connections continuously
void *slave_accept (void *args);
// Delete a disconnected slave
void slave_delete (int slave_fd);
// Run master server that communicate with slaves
int server_run ();


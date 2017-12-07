#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "util.h"
#include "proto_master_slave.h"

/*
typedef struct {
	uint8_t func_code;
	uint8_t response;
	char filename[FILENAME_LEN];
	size_t size;
	size_t offset;
	bool is_trunc;
	char buf[16 * K];
} FuseMsg;
*/

// fs related constants
#define FILENAME_LEN 	32
#define TEMP_DIR_NAME 		"slave_dir"

// internet address
#define DEAN_VM 			"fa17-cs241-083.cs.illinois.edu"
#define SERVER_PORT_STR 	"9001"

// socket to communicate with master
static int g_inet_sock = -1;

// local space
static FuseMsg* g_pt_msg = NULL;

// helper functions 
void initialize_global_data(void);
void close_slave(void);

void connect_to_master();

void switch_to_work_dir(void);
void send_all_current_files(void);

// start our client
int main(int argc, char** argv){
	// malloc and other stuff
	initialize_global_data();
	// connect to master
	connect_to_master();
	// slave directory
	switch_to_work_dir();

	// get and send current file names
	send_all_current_files();


	close_slave();
}

// switch to working directory
void switch_to_work_dir(void){
	int ret_val = chdir(TEMP_DIR_NAME);
	// create directory if none
	if(ret_val == -1){
		fprintf(stderr, "Creating directory: [%s]\n", TEMP_DIR_NAME);
		mkdir(TEMP_DIR_NAME, 0777);
		chdir(TEMP_DIR_NAME);
	}
}

// get and send current file names
void send_all_current_files(void){
	FILE *fp = popen("ls", "r");
	if(fp == NULL){
		perror("popen(ls)");
		return;
	}

	char file_list[65536];
	file_list[0] = '\0';

	size_t file_list_size = 0;

	// read from 
	char line_buf[2 * FILENAME_LEN];
	while (fgets(line_buf, sizeof(line_buf), fp)) {
		strcat(file_list, line_buf);
		file_list_size += strlen(line_buf);
	}


	printf("%s\n", file_list);
	printf("%lu\n", file_list_size);
}

// global data sections
void initialize_global_data(void){
	g_pt_msg = malloc(sizeof(FuseMsg));
}

void close_client(void){
	free(g_pt_msg);
}
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "network_utils.h"
#include "protocol.h"
#include "types.h"

// fs related constants
#define TEMP_DIR_NAME 		"slave_dir"

// internet address
#define DEAN_VM 			"fa17-cs241-083.cs.illinois.edu"
#define SERVER_PORT_STR 	"9001"

// socket to communicate with master
static int sock_fd = -1;

// local space
static FuseMsg* g_pt_msg = NULL;

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


//	printf("%s\n", file_list);
//	printf("%lu\n", file_list_size);

	write(sock_fd, file_list, 65536);
}

// global data sections
void initialize_global_data(void){
	g_pt_msg = malloc(sizeof(FuseMsg));
}

void close_client(void){
	free(g_pt_msg);
}

int connect_to_master () {
	int error = 0;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;  // TCP

	error = getaddrinfo(DEAN_VM, SERVER_PORT_STR, &hints, &res);
		if (error) {
			printf("Failed getaddrinfo(): %s\n", gai_strerror(error));
			return -1;
		}
	sock_fd = socket(res->ai_family, res->ai_socktype, 0);
		if (sock_fd == -1) {
			printf("Failed socket()\n");
			return -1;
		}
	error = connect(sock_fd, res->ai_addr, res->ai_addrlen);
		if (error == -1) {
			printf("Failed connect()\n");
			return -1;
		}
	
	return 0;
}

void process_get () {
	g_pt_msg->func_code = FUNC_RESP;
	g_pt_msg->response = RESP_SUCCEED;
	int fd = open(g_pt_msg->filename, O_RDONLY);
	pread(fd, g_pt_msg->buf, g_pt_msg->size, g_pt_msg->offset);
	close(fd);
}

void process_put () {
	g_pt_msg->func_code = FUNC_RESP;
	g_pt_msg->response = RESP_SUCCEED;
	int fd = open(g_pt_msg->filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	pwrite(fd, g_pt_msg->buf, g_pt_msg->size, g_pt_msg->offset);
	close(fd);
}

void process_del () {
	g_pt_msg->func_code = FUNC_RESP;
	g_pt_msg->response = RESP_SUCCEED;
}

void process_stat () {
	g_pt_msg->func_code = FUNC_RESP;
	g_pt_msg->response = RESP_SUCCEED;
	int ret = stat(g_pt_msg->filename, (struct stat *)g_pt_msg->buf);
	if (ret) g_pt_msg->response = RESP_FAILED;
}

void process_request () {
	memset(g_pt_msg, 0, sizeof(FuseMsg));
	read_all_from_socket(sock_fd, (char *)g_pt_msg, sizeof(FuseMsg));
	switch (g_pt_msg->func_code) {
	case FUNC_GET:
		process_get();
		break;
	case FUNC_PUT:
		process_put();
		break;
	case FUNC_DEL:
		process_del();
		break;
	case FUNC_STAT:
		process_stat();
		break;
	default:
		break;
	}
//	printf("%d %d %s %d %d %d %d %s\n", g_pt_msg->func_code, g_pt_msg->response, g_pt_msg->filename, g_pt_msg->size, g_pt_msg->size, g_pt_msg->offset, g_pt_msg->is_trunc, g_pt_msg->buf);
	write_all_to_socket(sock_fd, (char *)g_pt_msg, sizeof(FuseMsg));
}

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

	while (1) {
		process_request();
	}

	return 0;
}


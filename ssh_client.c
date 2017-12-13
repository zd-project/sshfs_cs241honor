#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "proto_client_master.h"
#include "util.h"
// internet constants
#define DEAN_VM 			"fa17-cs241-083.cs.illinois.edu"
#define SERVER_PORT_STR 	"10086"
#define DEAN_PC_IP 			"192.168.1.2"
#define SERVER_PORT 		10086
// cmds
// usage: exit
#define SHELL_CMD_EXIT 		"exit"
// usage: upload [file_name]
#define SHELL_CMD_UPLOAD	"upload"
// usage: download [file_name]
#define SHELL_CMD_DOWNLOAD 	"download"
// usage: exe [cmd] <additional cmd>
#define SHELL_CMD_EXECUTE	"exe"

// utils numer
#define MAX_FILE_NAME_LENGTH 	31

// flags
#define DEBUG 				false
#define GET_ADDR_INFO 		true

// data transmit field
static FILE_TRANSMIT* 	g_pt_file_upload;
static FILE_TRANSMIT* 	g_pt_file_download;
static EXECUTE_CMD* 	g_pt_execute_cmd;
static FEEDBACK* 		g_pt_feedback;
static HEARTBEAT* 		g_pt_heartbeat;
// socket
static volatile int g_sock;

// initialize all the global data field
void set_up_data_fields(void){
	// file upload struct
	g_pt_file_upload = malloc(sizeof(FILE_TRANSMIT));
	memset(g_pt_file_upload, 0 , sizeof(FILE_TRANSMIT));
	g_pt_file_upload->meta_data.func_code = FUNC_FILE_UPLOAD;

	// file download struct
	g_pt_file_download = malloc(sizeof(FILE_TRANSMIT));
	memset(g_pt_file_download, 0, sizeof(FILE_TRANSMIT));
	g_pt_file_download->meta_data.func_code = FUNC_FILE_DOWNLOAD;
	// will potentially be reset

	// execute command struct
	g_pt_execute_cmd = malloc(sizeof(EXECUTE_CMD));
	memset(g_pt_execute_cmd, 0, sizeof(EXECUTE_CMD));
	g_pt_execute_cmd->meta_data.func_code = FUNC_EXECUTE_CMD;

	// feedback field
	g_pt_feedback = malloc(sizeof(FEEDBACK));
	memset(g_pt_feedback, 0, sizeof(FEEDBACK));
	g_pt_feedback->meta_data.func_code = FUNC_FEEDBACK;

	// heartbeat field
	g_pt_heartbeat = malloc(sizeof(HEARTBEAT));
	memset(g_pt_heartbeat, 0, sizeof(HEARTBEAT));
	g_pt_heartbeat->meta_data.func_code = FUNC_HEARTBEAT;
}

// free all the heap data field
void free_up_data_fields(void){
	free(g_pt_file_upload);
	free(g_pt_file_download);
	free(g_pt_execute_cmd);
	free(g_pt_feedback);
	free(g_pt_heartbeat);
}

// connect to server
bool set_up_server_connection(void){
	g_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(g_sock < 0){
		perror("sock failed");
		return false;
	}else{
		fprintf(stdout, "mpssh run on socket: %d\n", g_sock);
	}

	int ret_code = 0;

	if(GET_ADDR_INFO == true){
		// get address hints
	    struct addrinfo hints, *result;
	    memset(&hints, 0, sizeof(struct addrinfo));
	    hints.ai_family = AF_INET;
	    hints.ai_socktype = SOCK_STREAM;
	    // get addr info
	    ret_code = getaddrinfo(DEAN_VM, SERVER_PORT_STR, &hints, &result);
	    if (ret_code != 0) {
	        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(ret_code));
	        return false;
	    }

	     // connect
	    ret_code = connect(g_sock, result->ai_addr, result->ai_addrlen);
	    if(ret_code < 0){
	        perror("connect() failed");
	        return false;
	    }

	    freeaddrinfo(result);
	}else{
		printf("start connection\n");
		struct sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr(DEAN_PC_IP);
		server.sin_port = htons(SERVER_PORT);

		ret_code = connect(g_sock, (const struct sockaddr*) &server, sizeof(server));
		if(ret_code < 0){
			perror("connect() failed");
			return false;
		}
	}

	printf("Connection done\n"); 

	return true;
}

// upload file to the server
bool read_file_and_upload(char* file_name){
	if(strlen(file_name) > 31){
		printf("file name too long!\n");
		return false;
	}
	printf("Upload file: %s\n", file_name);
	// get the file and its size
	FILE* f = fopen(file_name, "r");
	if(f == NULL){
		perror("Upload failed");
		return false;
	}
	// get file size
	fseek(f, 0L, SEEK_END);
	long file_size = ftell(f);
	rewind(f);

	// get the data ready
	g_pt_file_upload->meta_data.data_length = (MAX_FILE_NAME_LENGTH + 1) + file_size;
	strncpy(g_pt_file_upload->file_name, file_name, MAX_FILE_NAME_LENGTH);
	g_pt_file_upload->file_name[MAX_FILE_NAME_LENGTH] = '\0';
	fread(g_pt_file_upload->buffer, sizeof(char), file_size, f);

	printf("Func code: %d\n", g_pt_file_upload->meta_data.func_code);
	printf("Data len: %u\n", g_pt_file_upload->meta_data.data_length);
	printf("File name: %s\n", g_pt_file_upload->file_name);
	printf("File content:\n%s", g_pt_file_upload->buffer);

	// send the data accross
	ssize_t ret_code = write_all_to_socket(g_sock, (char*) g_pt_file_upload, sizeof(FILE_TRANSMIT));
	if(ret_code < 0){
		perror("Upload failed");
		return false;
	}
	/*printf("write bytes: %ld\n", ret_code);*/

	fclose(f);
	printf("File: [%s] uploaded to the server\n", file_name);
	return true;
}

// upload cmd to server
bool send_cmd_to_server(char* cmd_to_server){
	g_pt_execute_cmd->meta_data.data_length = strlen(cmd_to_server) + 1;
	strcpy(g_pt_execute_cmd->command, cmd_to_server);

	/*printf("Func code: %d\n", g_pt_execute_cmd->meta_data.func_code);*/
	/*printf("Data len: %u\n", g_pt_execute_cmd->meta_data.data_length);*/
	/*printf("CMD: [%s]\n", g_pt_execute_cmd->command);*/

	ssize_t ret_code = write_all_to_socket(g_sock, (char*) g_pt_execute_cmd, sizeof(EXECUTE_CMD));
	if(ret_code < 0){
		perror("send() failed");
		return false;
	}
	/*printf("write bytes: %ld\n", ret_code);*/

	printf("CMD: [%s] sent to the server\n", cmd_to_server);
	return true;
}

// receive the feedback
void receive_feedback_and_print(void){
	printf("Wait for feedback\n");
	ssize_t ret_code = read_all_from_socket(g_sock, (char*) g_pt_feedback, sizeof(FEEDBACK));
	if(ret_code < 0){
		perror("recv() failed");
		return;
	}
	/*printf("read bytes: %ld\n", ret_code);*/
	
	// extract the string
	if(g_pt_feedback->meta_data.func_code != FUNC_FEEDBACK){
		fprintf(stdout, "Wrong func code: %d\n", g_pt_feedback->meta_data.func_code);
		return;
	}
	fprintf(stdout, "Server: %s\n", g_pt_feedback->feedback);
}

// try download the file
void download_file(char* file_name){
	if(strlen(file_name) > 31){
		printf("file name too long!\n");
		return;
	}
	printf("download file: [%s]\n", file_name);

	// get the request body ready
	g_pt_file_download->meta_data.func_code = FUNC_FILE_DOWNLOAD;
	g_pt_file_download->meta_data.data_length = MAX_FILE_NAME_LENGTH + 1;
	strncpy(g_pt_file_download->file_name, file_name, MAX_FILE_NAME_LENGTH);
	g_pt_file_download->file_name[MAX_FILE_NAME_LENGTH] = '\0';

	ssize_t ret_code = write_all_to_socket(g_sock, (char*) g_pt_file_download, sizeof(FILE_TRANSMIT));
	if(ret_code < 0){
		perror("send() failed");
		return;
	}
	printf("write bytes: %ld\n", ret_code);

	ret_code = read_all_from_socket(g_sock, (char*) g_pt_file_download, sizeof(FILE_TRANSMIT));
	if(ret_code < 0){
		perror("recv() failed");
		return;
	}
	printf("read bytes: %ld\n", ret_code);

	if(g_pt_file_download->meta_data.func_code == FUNC_FEEDBACK){
		fprintf(stdout, "File [%s] doesn't exit\n", file_name);
		return;
	}

	printf("Data len: %d\n", g_pt_file_download->meta_data.data_length);
	printf("File name: %s\n", g_pt_file_download->file_name);
	printf("File content:\n%s", g_pt_file_download->buffer);

	// open the file, clear the content and write to it
	size_t file_length = ((size_t) g_pt_file_download->meta_data.data_length) - (MAX_FILE_NAME_LENGTH + 1);
	FILE* f = fopen(file_name, "w");
	fwrite(g_pt_file_download->buffer, sizeof(char), file_length, f);
	fclose(f);

	fprintf(stdout, "Download [%s] complete\n", file_name);
}

// cmd process entry
void process_cmd_in(char* shell_cmd_in){
	printf("cmd accepted: %s\n", shell_cmd_in);
	size_t cmd_word_count = 0;
	char** command = strsplit(shell_cmd_in, 10, &cmd_word_count);
	/*printf("word count: %lu\n", cmd_word_count);*/
	if(cmd_word_count == 0){
		// print usage
		return;
	}

	bool ret_bool;
	// if it is upload
	if(strcmp(command[0], SHELL_CMD_UPLOAD) == 0){
		if(cmd_word_count == 1){
			// print usage
			return;
		}else{
			ret_bool = read_file_and_upload(command[1]);
			if(ret_bool == true){
				receive_feedback_and_print();
			}
		}
	}
	// if it is download
	if(strcmp(command[0], SHELL_CMD_DOWNLOAD) == 0){
		if(cmd_word_count == 1){
			// print usage
			return;
		}else{
			download_file(command[1]);
		}
	}

	if(strcmp(command[0], SHELL_CMD_EXECUTE) == 0){
		ret_bool = send_cmd_to_server(shell_cmd_in + 4);
		if(ret_bool == true){
			receive_feedback_and_print();
		}
	}

	free_args(command);
}

int main(int argc, char** argv){
	// initialization
	set_up_data_fields();
	// try connect if fail terminate the shell
	if(DEBUG == false){
		bool ret_bool = set_up_server_connection();
		if(ret_bool == false){
			return 0;
		}
	}

	char* shell_cmd_in = NULL;
	size_t shell_cmd_size = 0;

	// start the shell
	while(true){
		// print prompt
		print_shell_prompt();
		// get the shell command in
		getline(&shell_cmd_in, &shell_cmd_size, stdin);
		shell_cmd_in[strlen(shell_cmd_in) - 1] = '\0';
		printf("cmd read: %s\n", shell_cmd_in);

		if(strcmp(shell_cmd_in, SHELL_CMD_EXIT) == 0){
			break;
		}
		// then process
		process_cmd_in(shell_cmd_in);
	}

	// finish up
	free_up_data_fields();
	free(shell_cmd_in);
	close(g_sock);
	return 0;
}


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
#define DEAN_VM_IP 		"172.22.148.84"
#define DEAN_PC_IP 		"10.180.129.14"
#define USED_IP	   		DEAN_VM_IP
#define SERVER_PORT 	9001
// cmds
// usage: exit
#define SHELL_CMD_EXIT 		"exit"
// usage: upload [file_name]
#define SHELL_CMD_UPLOAD	"upload"
// usage: download [file_name]
#define SHELL_CMD_DOWNLOAD 	"download"
// usage: exe [cmd] <additional cmd>
#define SHELL_CMD_EXECUTE	"exe"


// debug flag
#define DEBUG false

// data transmit field
static FILE_TRANSMIT* g_pt_file_upload;
static FILE_TRANSMIT* g_pt_file_download;
static EXECUTE_CMD* g_pt_execute_cmd;
static FEEDBACK* g_pt_feedback;
static int g_sock;

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

	// execute command struct
	g_pt_execute_cmd = malloc(sizeof(EXECUTE_CMD));
	memset(g_pt_execute_cmd, 0, sizeof(EXECUTE_CMD));
	g_pt_execute_cmd->meta_data.func_code = FUNC_EXECUTE_CMD;

	// feedback field
	g_pt_feedback = malloc(sizeof(FEEDBACK));
	memset(g_pt_feedback, 0, sizeof(FEEDBACK));
	g_pt_feedback->meta_data.func_code = FUNC_FEEDBACK;
}

void free_up_data_fields(void){
	free(g_pt_file_upload);
	free(g_pt_file_download);
	free(g_pt_execute_cmd);
	free(g_pt_feedback);
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

	char* server_ip = USED_IP;
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(server_ip);
	server.sin_port = htons(SERVER_PORT);

	int ret_code = connect(g_sock, (const struct sockaddr*) &server, sizeof(server));
	if(ret_code < 0){
		perror("connect failed");
		return false;
	}else{
		fprintf(stdout, "connect to server %s succeeded\n", server_ip);
	}
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
	long file_size = ftell(f) - 1;
	rewind(f);

	// get the data ready
	g_pt_file_upload->meta_data.data_length = sizeof(META_DATA) + file_size;
	strncpy(g_pt_file_upload->file_name, file_name, 31);
	g_pt_file_upload->file_name[strlen(file_name)] = '\0';
	fread(g_pt_file_upload->buffer, sizeof(char), file_size, f);

	printf("Func code: %d\n", g_pt_file_upload->meta_data.func_code);
	printf("Data len: %u\n", g_pt_file_upload->meta_data.data_length);
	printf("File name: %s\n", g_pt_file_upload->file_name);
	printf("File content:\n%s\n", g_pt_file_upload->buffer);

	// send the data accross
	int ret_code = send(g_sock, g_pt_file_upload, sizeof(FILE_TRANSMIT), 0);
	if(ret_code < 0){
		perror("Upload failed");
		return false;
	}

	printf("File: [%s] uploaded to the server\n", file_name);
	return true;
}

// upload cmd to server
bool send_cmd_to_server(char* cmd_to_server){
	g_pt_execute_cmd->meta_data.data_length = strlen(cmd_to_server) + 1;
	strcpy(g_pt_execute_cmd->command, cmd_to_server);

	printf("Func code: %d\n", g_pt_execute_cmd->meta_data.func_code);
	printf("Data len: %u\n", g_pt_execute_cmd->meta_data.data_length);
	printf("CMD: [%s]\n", g_pt_execute_cmd->command);

	int ret_code = send(g_sock, g_pt_execute_cmd, sizeof(EXECUTE_CMD), 0);
	if(ret_code < 0){
		perror("Exe failed");
		return false;
	}

	printf("CMD: [%s] sent to the server\n", cmd_to_server);
	return true;
}

// receive the feedback
void receive_feedback_and_print(void){
	printf("Wait for feedback\n");
	int ret_code = recv(g_sock, g_pt_feedback, sizeof(FEEDBACK), 0);
	if(ret_code < 0){
		perror("recv failed");
		return;
	}
	
	// extract the string
	if(g_pt_feedback->meta_data.func_code != FUNC_FEEDBACK){
		return;
	}
	fprintf(stdout, "Server: %s\n", g_pt_feedback->feedback);
}

// try download the file
void download_file(char* file_name){
	
}

// cmd process entry
void process_cmd_in(char* shell_cmd_in){
	printf("cmd accepted: %s\n", shell_cmd_in);
	size_t cmd_word_count = 0;
	char** command = strsplit(shell_cmd_in, 10, &cmd_word_count);
	printf("word count: %lu\n", cmd_word_count);
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


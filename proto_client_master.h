#pragma once

#define K 		1024

typedef struct meta_data_t {
	// not including the length of meta_data
	unsigned 	data_length;
	int 		func_code;
} META_DATA;

// function code
#define FUNC_FILE_UPLOAD 	0
// download
#define FUNC_FILE_DOWNLOAD 	1
// to be used later
#define FUNC_EXECUTE_CMD 	2
// feedback
#define FUNC_FEEDBACK		3

// file transmission protocal
typedef struct file_transmit_t {
	META_DATA 	meta_data;	// funcCode = 0
	char		file_name[32];
	char 		buffer[16 * K];
} FILE_TRANSMIT;

// execute basic shell command
typedef struct execute_cmd_t {
	META_DATA 	meta_data;
	char 		command[1 * K];
} EXECUTE_CMD;

// feedback struct
typedef struct feedback_t
{
	META_DATA 	meta_data;
	char 		feedback[16 * K];
} FEEDBACK;

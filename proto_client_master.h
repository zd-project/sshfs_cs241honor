#pragma once

#define K 1024

typedef struct meta_data_t {
	unsigned 	data_length;
	int 		func_code;
} META_DATA;

// function code
#define FUNC_FILE_TRANSMIT 	0
// to be used later
#define FUNC_EXECUTE_CMD 	1

// file transmission protocal
typedef struct file_trans_t {
	META_DATA 	meta_data;	// funcCode = 0
	char		file_name[32];
	char 		buffer[16 * K];
} FILE_TRANSMIT;
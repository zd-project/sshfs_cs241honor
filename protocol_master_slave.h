#pragma once

#define K 1024

typedef struct {
	uint32_t len;  // length of subsequent message: (0,16K)
	char buf[16 * K];
} Message;

typedef struct {
	uint32_t len;
	uint32_t func_code;
	char buf[16 * K - sizeof(uint32_t)];
} MessageInput;

typedef struct {
	uint32_t len;
	uint32_t func_code;
	char cmd_buf[16 * K - sizeof(uint32_t)];
} MessageExecCmd;

typedef struct {
	uint32_t len;
	uint32_t func_code;
	char file_name[32];
	char file_buf[16 * K - sizeof(uint32_t) - sizeof(char[32])];
} MessageFileTransmit;

#define FUNC_FILE_TRANSMIT 0
#define FUNC_EXEC_CMD 1


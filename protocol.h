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

typedef struct {
	uint8_t func_code;
	uint8_t response;
	char filename[FILENAME_LEN];
	size_t size;
	size_t offset;
	bool is_trunc;
	char buf[16 * K];
} FuseMsg;

#define FUNC_GET 0
#define FUNC_PUT 1
#define FUNC_DEL 2
#define FUNC_STAT 3


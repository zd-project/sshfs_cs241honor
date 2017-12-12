#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

#define K 1024

typedef struct {
	uint8_t func_code;
	char file_name[FILENAME_LEN];
	char buf[16 * K];
} Message;

#define FUNC_EXEC 1
#define FUNC_UPLD 2
#define FUNC_DNLD 3

typedef struct {
	uint8_t func_code;
	uint8_t response;
	char filename[FILENAME_LEN];
	size_t size;
	size_t offset;
	bool is_trunc;
	char buf[16 * K];
} FuseMsg;

#define FUNC_RESP 1
#define FUNC_GET 2
#define FUNC_PUT 3
#define FUNC_DEL 4
#define FUNC_STAT 5

#define RESP_SUCCEED 0
#define RESP_FAILED 1


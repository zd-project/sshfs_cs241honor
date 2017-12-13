#pragma once

#include <stdint.h>

typedef uint8_t Slaveid;
#define SLAVE_MAX 256

#define FILENAME_LEN 32
#define FILE_PER_SLAVE 256
#define SLAVE_UNAVAILABLE FILE_PER_SLAVE

typedef uint16_t Hash;
#define FILEMGR_HASH_MAX 65536
#define FILEMGR_NOFILE 255


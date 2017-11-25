#include <stdint.h>

typedef uint8_t Slaveid;
#define SLAVE_MAX (1<<(sizeof(Slaveid)))

typedef uint16_t Hash;
#define FILEMGR_HASH_MAX (1<<(sizeof(Hash)))
#define FILEMGR_NOFILE (FILEMGR_HASH_MAX-1)


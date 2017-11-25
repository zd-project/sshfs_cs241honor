#include <stdint.h>

#define FILEINFO_NOFILE -1
#define FILEINFO_HASH_VALUE_MAX 65536

typedef struct {
	int8_t f2s[FILEINFO_HASH_VALUE_MAX];
} Fileinfo;

extern Fileinfo fileinfo;

void Fileinfo_init ();
uint16_t Fileinfo_hash (char* filename);
void Fileinfo_set (char* filename, int8_t slave_id);
uint8_t Fileinfo_get (char* filename);
void Fileinfo_init_slave (int8_t slave_id, int slave_fd);


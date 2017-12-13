#pragma once

#include "types.h"

typedef struct {
	Slaveid f2s[FILEMGR_HASH_MAX];
	char s2f[SLAVE_MAX][FILE_PER_SLAVE][FILENAME_LEN];
	uint8_t cnt[SLAVE_MAX];
} Filemgr;

extern Filemgr filemgr;

void Filemgr_init ();
void Filemgr_init_slave (Slaveid slave_id, int slave_fd);
Slaveid Filemgr_assign_slave(char *filename);
void Filemgr_set (char* filename, Slaveid slave_id);
Slaveid Filemgr_file_to_id (char* filename);
//char*** Filemgr_get_files ();
Hash Filemgr_hash (char* filename);

#include "types.h"

typedef struct {
	Slaveid f2s[FILEMGR_HASH_MAX];
} Filemgr;

extern Filemgr filemgr;

void Filemgr_init ();
void Filemgr_init_slave (Slaveid slave_id, int slave_fd);
void Filemgr_set (char* filename, Slaveid slave_id);
Slaveid Filemgr_get (char* filename);
Hash Filemgr_hash (char* filename);


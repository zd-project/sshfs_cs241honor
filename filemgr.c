#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "filemgr.h"
#include "types.h"

Filemgr filemgr;

void Filemgr_init () {
	memset(filemgr.f2s, FILEMGR_NOFILE, sizeof(filemgr.f2s));
}

void Filemgr_init_slave (Slaveid slave_id, int slave_fd) {
	char file_list[65536];
	int file_list_len = read(slave_fd, file_list, 65536);
	file_list[file_list_len] = '\0';
	
	char file_name[32];
	while (sscanf(file_list, "%s", file_name) != EOF) {
		Filemgr_set(file_name, slave_id);
	}
}

void Filemgr_set (char* filename, Slaveid slave_id) {
	Hash filehash = Filemgr_hash(filename);
	filemgr.f2s[filehash] = slave_id;
}

Slaveid Filemgr_get (char* filename) {
	Hash filehash = Filemgr_hash(filename);
	return filemgr.f2s[filehash];
}

Hash Filemgr_hash (char* filename) {
	Hash filehash = 0;
	while (*filename) {
		filehash = filehash * 31 + *(filename++);
	}
	return filehash;
}


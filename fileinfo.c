#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fileinfo.h"

Fileinfo fileinfo;

void Fileinfo_init () {
	memset(fileinfo.f2s, FILEINFO_NOFILE, sizeof(fileinfo.f2s));
}

uint16_t Fileinfo_hash (char* filename) {
	uint16_t ans = 0;
	while (*filename) {
		ans = ans * 31 + *(filename++);
	}
	return ans;
}

void Fileinfo_set (char* filename, int8_t slave_id) {
	uint16_t filehash = Fileinfo_hash(filename);
	fileinfo.f2s[filehash] = slave_id;
}

uint8_t Fileinfo_get (char* filename) {
	uint16_t filehash = Fileinfo_hash(filename);
	return fileinfo.f2s[filehash];
}

void Fileinfo_init_slave (int8_t slave_id, int slave_fd) {
	char file_list[65536];
	int file_list_len = read(slave_fd, file_list, 65536);
	file_list[file_list_len] = '\0';
	
	char file_name[32];
	while (sscanf(file_list, "%s", file_name) != EOF) {
		Fileinfo_set(file_name, slave_id);
	}
}


#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "filemgr.h"
#include "fs_server.h"
#include "types.h"
#include "network_utils.h"
Filemgr filemgr;

void Filemgr_init () {
	memset(filemgr.f2s, FILEMGR_NOFILE, sizeof(filemgr.f2s));
	memset(filemgr.s2f, '\0', sizeof(filemgr.s2f));
	memset(filemgr.cnt, 0, sizeof(filemgr.f2s));
}

void Filemgr_init_slave (Slaveid slave_id, int slave_fd) {
	char file_list[65536];
	int file_list_len = read_all_from_socket(slave_fd, file_list, 65536);
//	file_list[file_list_len] = '\0';
	
    /*fprintf(stderr, "finish read\n");*/
	char file_name[FILENAME_LEN];
    char* file_list_ptr = &file_list[0];
    int read_size;
	while ((read_size = sscanf(file_list_ptr, "%s", file_name)) > 0) {
        /*fprintf(stderr, "read: %d\n", read_size);*/
        /*fprintf(stderr, "filename: %s\n", file_name);*/
        /*fprintf(stderr, "slave id: %d\n", slave_id);*/
		Filemgr_set(file_name, slave_id);
        /*fprintf(stderr, "finish set\n");*/
        file_list_ptr += strlen(file_name) + 1;
	}
}

Slaveid Filemgr_assign_slave(char *filename) {
    // first fit policy
    for (Slaveid id = 0; id < SLAVE_MAX; id++) {
        if (slaves[id].active && filemgr.cnt[id] != SLAVE_UNAVAILABLE) {
            Filemgr_set(filename, id);
            return id;
        }
    }
    return -1;
}

void Filemgr_set (char* filename, Slaveid slave_id) {
    /*fprintf(stderr, "FILEMGR_HASH_MAX\n");*/
	Hash filehash = Filemgr_hash(filename);
	filemgr.f2s[filehash] = slave_id;
	memcpy(filemgr.s2f[slave_id][filemgr.cnt[slave_id]], filename, strlen(filename) + 1);
	filemgr.cnt[slave_id] ++;
}

Slaveid Filemgr_file_to_id (char* filename) {
	Hash filehash = Filemgr_hash(filename);
    /*fprintf(stderr, "finish hash\n");*/
	Slaveid id = filemgr.f2s[filehash];
    if (slaves[id].active) {
        return id;
    } else {
        return FILEMGR_NOFILE;
    }
}
/*
char*** Filemgr_get_files () {
    char ***file_matrix = malloc(sizeof(char **) * (1 + SLAVE_MAX));
    int cnt = 0;
    for (Slaveid slave_id = 0; slave_id < SLAVE_MAX; slave_id++) {
        if (slaves[slave_id].active) {
            file_matrix[cnt++] = filemgr.s2f[slave_id];
        }
        file_matrix[cnt] = NULL;
    }
    return file_matrix;
}
*/
Hash Filemgr_hash (char* filename) {
	Hash filehash = 0;
	while (*filename) {
		filehash = filehash * 31 + *(filename++);
	}
	return filehash;
}


/* Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall myfs.c `pkg-config fuse3 --cflags --libs` -o myfs
 *
 * ## Source code ##
 * \include myfs.c
 */

#define FUSE_USE_VERSION 32

//#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "fs_server.h"
#include "protocol.h"
#include "types.h"

#include "filemgr.h"
#include "fs_server.h"
#include "protocol.h"
#include "network_utils.h"


static int myfs_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    (void) fi;
    printf("get attr\n");
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char *filename = (char*)path + 1;
        Slaveid sid = Filemgr_file_to_id(filename);
        if (sid == FILEMGR_NOFILE) {
            fprintf(stderr, "slave not available\n");
            return -ENOENT;
        }

        int sfd = slaves[sid].fd;
        
        FuseMsg* msg = calloc(1, sizeof(FuseMsg));
        msg->func_code = FUNC_STAT;
        fprintf(stderr, "getattr\n");
        fprintf(stderr, "%s\n", filename);
        memcpy(msg->filename, filename, strlen(filename));
        if (write_all_to_socket(sfd, (char*)msg, sizeof(FuseMsg)) < 1) {
            perror("request failure");
            return -EACCES;
        }
        if (!slaves[sid].active) {
            fprintf(stderr, "slave died\n");
            return -ENOENT;
        }

        memset(msg, 0, sizeof(FuseMsg));
        if (read_all_from_socket(sfd, (char*)msg, sizeof(FuseMsg)) < 1) {
            perror("read failure");
            return -EACCES;
        }

        if (msg->response == RESP_FAILED) {
            fprintf(stderr, "failed to get file stat\n");
            return -ENOENT;
        }

        struct stat* file_stat = (struct stat *)msg->buf;
        memcpy(stbuf, file_stat, sizeof(struct stat));
        stbuf->st_uid = getuid(); 
        stbuf->st_gid = getgid();
        stbuf->st_ino = 0;
        stbuf->st_mode |= S_IFREG | 0644;
        stbuf->st_nlink = 1;
        return 0;
    }
    return 0;
}

static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi,
             enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;
    printf("enter readdir\n");
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    printf("%s\n", path);
    /*char* full_path = calloc(strlen("slave_dir")+1,1);*/
    /*full_path = strcat(full_path, "/");*/
    /*full_path = strcat(full_path, "slave_dir");*/
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
	for (Slaveid slave_id = 0; slave_id < SLAVE_MAX-1; slave_id++) {
		for (uint8_t i = 0; i < filemgr.cnt[slave_id]; i++) {
			filler(buf, filemgr.s2f[slave_id][i], NULL, 0, 0);
		}
	}
    printf("done with readdir\n");
	/*
    char*** file_matrix = Filemgr_get_files();
    while (*file_matrix) {
        char **slave_files = *file_matrix;
        while (*slave_files) {
            filler(buf, *slave_files, NULL, 0, 0);
            slave_files++;
        }
        file_matrix++;
    }
	*/
    return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi) {
    // send stat to slave
    printf("enter open\n");
    char* filename = (char*)++path;
    Slaveid sid = Filemgr_file_to_id(filename);
    if (sid == FILEMGR_NOFILE) {
        fprintf(stderr, "slave does not exist\n");
        return -ENOENT;
    }
    int sfd = slaves[sid].fd;

    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_STAT;
    fprintf(stderr, "%s\n", filename);
    memcpy(msg -> filename, filename, strlen(filename));
    ssize_t bytes_write = write_all_to_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }
     
    memset(msg, 0, sizeof(FuseMsg));
    if (!slaves[sid].active) {
        fprintf(stderr, "slave died\n");
        return -EACCES;
    }
    size_t bytes_read = read_all_from_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "failed to get file stat\n");
        return -EACCES;
    }
     
    // check mode
    struct stat* file_stat = (struct stat*) msg -> buf;
    int user_want_read = fi -> flags & O_ACCMODE & O_RDONLY || fi -> flags & O_ACCMODE & O_RDWR;
    int user_want_write =  fi -> flags & O_ACCMODE & O_WRONLY || fi -> flags & O_ACCMODE & O_RDWR;
    if ((!(file_stat -> st_mode & S_IRUSR)) && user_want_read) {
        // you don't have read access but you want
        printf("you don't have read permission\n");
        return -EACCES;
    } 
    else if ((!(file_stat -> st_mode & S_IWUSR)) && user_want_write) {
        // want to write but don't have permission
        printf("u don't have write permission\n");
        return -EACCES;
    }
    free(msg);
    msg = NULL;
    return 0;
}

static int myfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    if (!size) return 0;
    printf("enter read\n");
    char* filename = (char*)++path;
    Slaveid sid = Filemgr_file_to_id(filename);

    if (sid == FILEMGR_NOFILE) {
        fprintf(stderr, "slave does not exist\n");
        return -EIO;
    }
    int sfd = slaves[sid].fd; 

    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        return -EIO;
    }

    FuseMsg *msg = calloc(1, sizeof(FuseMsg));
    msg->func_code = FUNC_GET;
    msg->offset = offset;
    msg->size = size;
    memcpy(msg->filename, filename, strlen(filename));
    if (write_all_to_socket(sfd, (char*)msg, sizeof(FuseMsg)) < 1) {
        perror("request failure");
        free(msg);
        return -EIO;
    }
    if (!slaves[sid].active) {
        fprintf(stderr, "slave died\n");
        free(msg);
        return -EIO;
    }

    memset(msg, 0, sizeof(FuseMsg));
    ssize_t bytes_read;
    bytes_read = read_all_from_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_read < 1) {
        perror("read failure");
        free(msg);
        return -EIO;
    }

    if (msg->response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -EIO;
    } else {
        memcpy(buf, msg->buf, msg->size);
        printf("buff: %s\n", msg->buf);
        return size;
    }
}

static int myfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
    // get a random slave_sock from file manager
    printf("enter create\n");
    char* filename = (char*)++path;
    Slaveid sid = Filemgr_assign_slave(filename);

    if (sid == FILEMGR_NOFILE) {
        fprintf(stderr, "slave does not exist\n");
        return -ENOENT;
    }
    int sfd = slaves[sid].fd; 

    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        return -EACCES;
    }

    // send PUT request to slave_sock 
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    memcpy(msg->filename, filename, strlen(filename)); 

    ssize_t bytes_write = write_all_to_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }

    memset(msg, 0, sizeof(FuseMsg));
    ssize_t bytes_read = read_all_from_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -EACCES;
    }
    return 0;
    // we want file mode, need an extra struct for file_create
}

static int myfs_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
    
    //TODO qeustion, buffer is a char*? what if the file content has \0 in the
    // middle?
    // get slave_sock that has the file on it 
    printf("enter write\n");
    char* filename = (char*)++path;
    Slaveid sid = Filemgr_file_to_id(filename); 
    int sfd = slaves[sid].fd; 
    
    // send PUT request to slave_sock offset??  if has offset, with offset. 
    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        return -EACCES;
    }
    
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    memcpy(msg->buf, buffer, size);
    // NOT SURE
    msg -> is_trunc = false;
    msg -> offset = offset;
    msg -> size = size;
    memcpy(msg->filename, filename, strlen(filename));

    ssize_t bytes_write = write_all_to_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }
    
    memset(msg, 0, sizeof(FuseMsg));
    ssize_t bytes_read = read_all_from_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -EACCES;
    }

    return bytes_write;
}

static int myfs_truncate(const char* path, off_t offset, struct fuse_file_info* fi) { 

    // check permssion (if you don't have write permission)
     
    // send PUT request with offset 0 truncate true
    printf("enter trunc\n");
    char* filename = (char*)++path;
    Slaveid sid = Filemgr_file_to_id(filename); 
    int sfd = slaves[sid].fd; 
    
    // send PUT request to slave_sock offset??  if has offset, with offset. 
    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        return -EACCES;
    }
    
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    msg -> is_trunc = true;
    memcpy(msg->filename, filename, strlen(filename));
    ssize_t bytes_write = write_all_to_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }
    //TODO
    memset(msg, 0, sizeof(FuseMsg));
    ssize_t bytes_read = read_all_from_socket(sfd, (char*)msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -EACCES;
    }
    return 0;
}

static struct fuse_operations myfs_oper = {
//    .init           = myfs_init,
    .getattr    = myfs_getattr,
    /*.setattr    = myfs_setattr,*/
    .readdir    = myfs_readdir,
    .open       = myfs_open,
    .read       = myfs_read,
    .create     = myfs_create,
    .write      = myfs_write,
    .truncate   = myfs_truncate,
};

int main(int argc, char *argv[])
{
    server_run();
    return fuse_main(argc, argv, &myfs_oper, NULL);
}

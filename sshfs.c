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
#include <sshfs_utils.h>
#include <stdbool.h>

#include "types.h"
#include "filemgr.h"
#include "fs_server.h"
#include "protocol.h"
#include "network_utils.h"

static int myfs_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    (void) fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        const char *filename = path + 1;
        Slaveid sid = Filemgr_file_to_id(filename);
        if (sid == FILEMGR_NOFILE) {
            printf(stderr, "slave not available\n");
            return -ENOENT;
        }

        int slaveFd = slaves[sid].fd;

        FuseMsg* statRequest = calloc(1, sizeof(FuseMsg));
        msg->func_code = FUNC_STAT;
        msg->filename = filename;
        if (write_all_to_socket(sfd, msg, sizeof(FuseMsg)) < 1) {
            perror("request failure");
            return -EACCES;
        }
        if (!slaves[sid].active) {
            fprintf(stderr, "slave died\n");
            return -ENOENT;
        }

        memset(msg, 0, sizeof(FuseMsg));
        if (read_all_from_socket(sfd, msg, sizeof(FuseMsg)) < 1) {
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
}

static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi,
             enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    char*** file_matrix = Filemgr_get_files (Slaveid slave_id);
    while (*file_matrix) {
        char **slave_files = *file_matrix;
        while (*slave_files) {
            filler(buf, *slave_files, NULL, 0, 0);
            slave_files++;
        }
        file_matrix++;
    }

    return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi) {
    // send stat to slave
    char* filename = path++;
    Slaveid sid = Filemgr_file_to_id(filename);
    if (sid == FILEMGR_NOFILE) {
        printf(stderr, "slave does not exist\n");
        return -ENOENT;
    }
    int sfd = slaves[sid].fd;

    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_STAT;
    msg -> filename = filename;
    ssize_t bytes_write = write_all_to_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }
     
    memset(msg, 0, sizeof(FuseMsg));
    if (!slaves[sid].active) {
        fprintf(stderr, "slave died\n");
        return -1;
    }
    size_t bytes_read = read_all_from_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "failed to get file stat\n");
        return -1;
    }
     
    // check mode
    stat* file_stat = (stat*) msg;
    int user_want_read = fi -> flags & O_ACCESS & O_RDONLY || fi -> flags & O_ACCESS & O_RDWR;
    if ((!(file_stat -> st_mode & S_IRUSR)) && user_want_read) {
        // you don't have read access but you want
        return -EACCES;
    } 
    int user_want_write =  fi -> flags & O_ACCESS & O_WRONLY || fi -> flags & O_ACCESS & O_RDWR;
    else if ((!(file_stat -> st_mode & S_IWUSR)) && user_want_write) {
        // want to write but don't have permission
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

    char* filename = path++;
    Slaveid id = Filemgr_file_to_id(filename);

    if (id == FILEMGR_NOFILE) {
        printf(stderr, "slave does not exist\n");
        return -EIO;
    }
    int sfd = slaves[sid].fd; 

    if (!slaves[id].active) {
        fprintf(stderr, "slave already died\n");
        return -EIO;
    }

    FuseMsg *msg = calloc(1, sizeof(FuseMsg));
    msg->func_code = FUNC_GET;
    msg->offset = offset;
    msg->size = size;

    if (write_all_to_socket(sfd, msg, sizeof(FuseMsg)) < 1) {
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
    if (read_all_from_socket(sfd, msg, sizeof(FuseMsg)) < 1) {
        perror("read failure");
        free(msg);
        return -EIO;
    }

    if (msg->response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -EIO;
    } else {
        memcpy(buf, mes->buf, msg->size);
        return 0;
    }
}

static int myfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
    // get a random slave_sock from file manager
    char* filename = path++;
    Slaveid sid = Filemgr_assign_slave(filename);

    if (sid == FILEMGR_NOFILE) {
        printf(stderr, "slave does not exist\n");
        return -ENOENT;
    }
    int sfd = slaves[sid].fd; 

    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        free(msg);
        return -1;
    }

    // send PUT request to slave_sock 
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    msg -> filename = filename; 

    ssize_t bytes_write = write_all_to_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }

    memset(msg, 0, sizeof(FuseMsg));
    ssize_t bytes_read = read_all_from_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -1;
    }
    return 0;
    // we want file mode, need an extra struct for file_create
}

static int myfs_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
    
    //TODO qeustion, buffer is a char*? what if the file content has \0 in the
    // middle?
    // get slave_sock that has the file on it 
    char* filename = path++;
    Slaveid sid = Filemgr_file_to_id(filename); 
    int fsd = slaves[sid].fd; 
    
    // send PUT request to slave_sock offset??  if has offset, with offset. 
    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        free(msg);
        return -1;
    }
    
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    msg -> buff = buffer;
    // NOT SURE
    msg -> is_trunc = false;
    msg -> offset = offset;

    msg -> size = size;

    ssize_t bytes_write = write_all_to_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }
    
    memset(msg, 0, sizeof(FuseMsg));
    ssize_t bytes_read = read_all_from_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "fail to create file\n");
        free(msg);
        return -1;
    }

    return bytes_write;
}

static int myfs_truncate(const char* path, off_t offset, struct fuse_file_info* fi) { 

    // check permssion (if you don't have write permission)
     
    // send PUT request with offset 0 truncate true
    char* filenmae = path++;
    Slaveid sid = Filemgr_file_to_id(filename); 
    int fsd = slaves[sid].fd; 
    
    // send PUT request to slave_sock offset??  if has offset, with offset. 
    if (!slaves[sid].active) {
        fprintf(stderr, "slave already died\n");
        free(msg);
        return -1;
    }
    
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    msg -> is_trunc = true;

    ssize_t bytes_write = write_all_to_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_write == -1) {
        perror(NULL);
        return -ENOENT;
    }
    return 0;
}

static struct fuse_operations myfs_oper = {
//    .init           = myfs_init,
    .getattr    = myfs_getattr,
    .readdir    = myfs_readdir,
    .open       = myfs_open,
    .read       = myfs_read,
    .create     = myfs_create,
    .write      = myfs_write,
    .truncate   = myfs_truncate,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &myfs_oper, NULL);
}

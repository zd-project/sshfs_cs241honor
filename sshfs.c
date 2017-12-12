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
#include "fs_server.h"
#include "protocol.h"
#include "types.h"

const char *PORT = "16354";
char defaultFilename[1000] = "myfs.honor";
char defaultContent[1000] = "The first file\n";

void read_from_internet() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror(NULL);
        exit(1);
    }

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *result;
    int err = getaddrinfo(NULL, PORT, &hints, &result);
    if (err) {
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(1);
    }

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen)) {
        perror(NULL);
        exit(1);
    }

    if (listen(sock_fd, 10) != 0) {
        perror(NULL);
        exit(1);
    }
    
    printf("Syncing...\n");
    int client_fd = accept(sock_fd, NULL, NULL);
    if (client_fd == -1) {
        perror(NULL);
        exit(1);
    }

    int len;
    len = read(client_fd, defaultFilename, sizeof(defaultFilename));
    defaultFilename[len] = '\0';

    printf("Read files\n");
    printf("===\n");
    printf("%s\n", defaultFilename);

    len = read(client_fd, defaultContent, sizeof(defaultContent));
    defaultContent[len - 1] = '\n';
    defaultContent[len] = '\0';

    printf("Read content: %d chars\n", len);
    printf("===\n");
    printf("%s\n", defaultContent);

    if (shutdown(sock_fd, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(sock_fd);
    if (shutdown(client_fd, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(client_fd);

    freeaddrinfo(result);
}

static int myfs_open(const char *path, struct fuse_file_info *fi)
{
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
        return -EACCES;
    }
    size_t bytes_read = read_all_from_socket(sfd, msg, sizeof(FuseMsg));
    if (bytes_read == -1) {
        perror(NULL);
        return -ENOENT;
    }

    if (msg -> response == RESP_FAILED) {
        fprintf(stderr, "failed to get file stat\n");
        return -EACCES;
    }
     
    // check mode
    stat* file_stat = (stat*) msg -> buf;
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
        return -EACCES;
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
        return -EACCES;
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
        return -EACCES;
    }
    
    FuseMsg* msg = calloc(sizeof(FuseMsg), 1);
    msg -> func_code = FUNC_PUT;
    msg -> buf = memcpy(msg->buf, buffer, size);
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
        return -EACCES;
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
        return -EACCES;
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
    .open        = myfs_open,
    .read        = myfs_read,
    .create = myfs_create,
    .write = myfs_write,
    .truncate = myfs_truncate,
};

int main(int argc, char *argv[])
{
    server_run();
    return fuse_main(argc, argv, &myfs_oper, NULL);
}

/*
  FUSE: Filesystem in Userspace
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

//static void *myfs_init(struct fuse_conn_info *conn,
//			struct fuse_config *cfg)
//{
//	(void) conn;
//	cfg->kernel_cache = 1;
//	return NULL;
//}

static int myfs_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path+1, defaultFilename) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(defaultContent);
	} else
		res = -ENOENT;

	return res;
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
	filler(buf, defaultFilename, NULL, 0, 0);

	return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path+1, defaultFilename) != 0)
		return -ENOENT;

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int myfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path+1, defaultFilename) != 0)
		return -ENOENT;

	len = strlen(defaultContent);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, defaultContent + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations myfs_oper = {
//	.init           = myfs_init,
	.getattr	= myfs_getattr,
	.readdir	= myfs_readdir,
	.open		= myfs_open,
	.read		= myfs_read,
};

int main(int argc, char *argv[])
{
    if (!strcmp(argv[1], "-n")) {
        read_from_internet();
        argv[1] = argv[0];
        argc--;
        argv++;
    }
	return fuse_main(argc, argv, &myfs_oper, NULL);
}

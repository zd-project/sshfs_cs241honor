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

//static void *myfs_init(struct fuse_conn_info *conn,
//			struct fuse_config *cfg)
//{
//	(void) conn;
//	cfg->kernel_cache = 1;
//	return NULL;
//}
char *defaultFilename = "myfs.honor";
char *defaultContent = "The first file\n";

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
	return fuse_main(argc, argv, &myfs_oper, NULL);
}

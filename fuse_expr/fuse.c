#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

static int do_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if (!strcmp(path, "/"))
	{
		st->st_mode = S_IFDIR | 0755;
	}
	else
	{
		st->st_mode = S_IFREG | 0644;
		st->st_size = 1024;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	filler(buffer, ".", NULL, 0); // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory
	
	if (!strcmp(path, "/")) { // If the user is trying to show the files/directories of the root directory show the following 
		filler(buffer, "test", NULL, 0);
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	char test[] = "Hello World From File54!";
	char *selectedText = NULL;
	
	// ... //
	
	if (!strcmp(path, "/test"))
		selectedText = test;
	else
		return -1;
	
	// ... //
	
	memcpy(buffer, selectedText + offset, size);
		
	return strlen(selectedText) - offset;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}

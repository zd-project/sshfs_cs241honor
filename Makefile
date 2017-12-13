

fuse:
<<<<<<< HEAD
<<<<<<< HEAD
	gcc -Wall network_utils.c filemgr.c sshfs.c `pkg-config fuse3 --cflags --libs` -o sshfs
=======
	gcc -Wall fs_server.c filemgr.c network_utils.c sshfs.c `pkg-config fuse3 --cflags --libs` -o sshfs
>>>>>>> bug fixes
=======
	gcc -Wall fs_server.c filemgr.c network_utils.c sshfs.c `pkg-config fuse3 --cflags --libs` -o sshfs
>>>>>>> bug fixes

slave:
	gcc -std=gnu99 -o slave slave.c network_utils.c

clean:
	rm slave
	rm sshfs




fuse:
	gcc -Wall network_utils.c sshfs.c `pkg-config fuse3 --cflags --libs` -o sshfs

slave:
	gcc -std=gnu99 -o slave slave.c network_utils.c

clean:
	rm slave
	rm sshfs


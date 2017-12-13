client:
	gcc -std=gnu99 -o client ssh_client.c util.c 

server:
	gcc -std=gnu99 -o server tryServer1.c util.c

fuse:
	gcc -Wall fs_server.c filemgr.c network_utils.c sshfs.c `pkg-config fuse3 --cflags --libs` -o sshfs

slave:
	gcc -std=gnu99 -o slave slave.c network_utils.c

clean:
	rm slave
	rm sshfs


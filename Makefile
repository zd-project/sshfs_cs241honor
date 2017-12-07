

slave:
	gcc -std=gnu99 -o slave slave.c network_utils.c

clean:
	rm slave


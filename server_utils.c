#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include "server_utils.h"

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    ssize_t total_read = 0;
    while(total_read < (ssize_t)count) {
        // non-blocking receive
        ssize_t bytes_read = recv(socket, buffer+total_read, count - total_read, MSG_DONTWAIT);
        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            else if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    return total_read;
            }
            else {
                return -1;
            }
        }
        total_read += bytes_read;
        if (bytes_read == 0) {
            return total_read;
        }
    }
        return total_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t total_write = 0;
    while(total_write < (ssize_t)count) {
        ssize_t bytes_write = write(socket, buffer + total_write, count - total_write);
        if (bytes_write == -1) {
            if (errno == EINTR) {
                continue;
            }
            else {
                return -1;
            }
        }
        total_write += bytes_write;
        if (bytes_write == 0) {
            return total_write;
        }
        total_write += bytes_write;
        if (bytes_write == 0) {
            return total_write;
        }
    }
    return total_write;
}

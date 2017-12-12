/**
 * Chatroom Lab
 * CS 241 - Fall 2017
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "network_utils.h"

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    ssize_t totalread = 0;
    ssize_t bytesread = 0;
    ssize_t signedcount = count;
    while (signedcount > 0 && (bytesread = read(socket, buffer, signedcount))) {
        if (bytesread == -1) {
            if (errno != EINTR) {
                return -1;
            }
        } else {
            totalread += bytesread;
            signedcount -= bytesread;
            buffer += bytesread;
        }
    }
    return totalread;
}

ssize_t write_all_to_socket(int socket, char *buffer, size_t count) {
    ssize_t totalwrite = 0;
    ssize_t byteswrite = 0;
    ssize_t signedcount = count;
    while (signedcount > 0 && (byteswrite = write(socket, buffer, signedcount))) {
        if (byteswrite == -1) {
            if (errno != EINTR) {
                return -1;
            }
        } else {
            totalwrite += byteswrite;
            signedcount -= byteswrite;
            buffer += byteswrite;
        }
    }
    return totalwrite;
}

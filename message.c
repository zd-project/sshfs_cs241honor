#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "message.h"

//typedef struct message {
//    size_t funct;
//    size_t length;
//    char [] content;
//} message;

void serialize_int(uint32_t x, char *write) {
    /* assume int == long; how can this be done better? */
    x = htonl(x);
    memcpy(write, &x, sizeof(uint32_t));
}

int serialize(char **str_out, const message *msg_out) {
    if (!(*str_out = malloc(HINT_LEN + MULTIPLE * VAR_NUM + msg_out->length))) {
        perror("Error allocating memory");
        return 1;
    }
    char *cur = str_out;
    memcpy(cur, HINT, HINT_LEN);
    serialize_uint(msg_out->funct, (cur += HINT_LEN));
    serialize_uint(msg_out->length, (cur += MULTIPLE));
    memcpy((cur += MULTIPLE), msg_out->content, msg_out->length);
    return 0;
}

int deserialize(message **msg_in, const char *in) {
    assert(in);
    char *hint_chk = malloc(HINT_LEN + 1);
    memcpy(hint_chk, in, HINT_LEN);
    if (!strcmp(hint_chk, HINT)) {
        return 1;
        fprintf(stderr, "Corrupted data detected! Failed to deserialize string");
    }
    free(hint_chk);

    // padding?
    const uint32_t *cur = (uint32_t *)(in + HINT_LEN);
    uint32_t funct_code = ntohl(cur[0]);
    cur++;
    uint32_t content_length = ntohl(cur[1]);
    cur++;

    if (!(*msg_in = malloc(sizeof(message) + content_length))) {
        perror("Error allocating memory");
        return 1;
    }
    *msg_in->funct = funct_code;
    *msg_in->length = content_length;
    memcpy(msg_in->content, (void *)cur, content_length);

    return 0;
}

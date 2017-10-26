#include <string.h>

const size_t EXEC = 0x1;
const size_t UPLD = 0x2;
const size_t DNLD = 0x3;
const size_t SLVR = 0x0;
const char * HINT = "$>";
const char * HINT_LEN = strlen(HINT);

const int MULTIPLE = sizeof(size_t);
const size_t VAR_NUM = 2;
typedef struct message {
    int funct;
    int length;
    char [] content; // isn't null-terminated
} message;

int serialize(char **str_out, const message *msg_out);
int deserialize(message *msg_in, const char *in);



#pragma once

#define K 1024


typedef struct meta_data_t {
        // not included in the length of meta data
        unsigned        data_length;
        int             func_code;
} META_DATA;

// function code
// #define FUNC_FILE_TRANSMIT   

#define FUNC_FILE_UPLOAD        0

#define FUNC_FILE_DOWNLOAD      1
// to be used later
#define FUNC_EXECUTE_CMD        2
// feedback
#define FUNC_FEEDBACK           3

#define FUNC_HEARTBEAT          4
// file transmission protocal
typedef struct file_transmit_t {
        META_DATA       meta_data;      // funcCode = 0
        char            file_name[32];
        char            buffer[16 * K];
} FILE_TRANSMIT;


typedef struct execute_cmd_t {
        META_DATA       meta_data;      // funcCode = 0
        char            command[1 * K];
} EXECUTE_CMD;

typedef struct  feedback_t
{
        META_DATA       meta_data;
        char            feedback[16 * K + 32];
} FEEDBACK;

typedef struct  heartbeat_t {
        META_DATA meta_data;
        char            buffer[24];
}HEARTBEAT;
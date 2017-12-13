#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

// print the shell prompt
void print_shell_prompt(void){
	// get current directory and print
	char current_dir[256];
    getcwd(current_dir, 256);
    // print to stdout
	fprintf(stdout, "(mpssh) %s: ", current_dir);
	fflush(stdout);
}

// split the str by whitespace and deep copy, null ptr terminated
char **strsplit(const char *str, size_t expected_tokens, size_t* num_token){
	char** ret_str_array = calloc(expected_tokens + 1, sizeof(char*));
	int word_read_count = 0;
	int this_part_length = 0;
	const char* traverser = str;

	while(traverser[0] != '\0'){
		if(traverser[0] != ' '){
			this_part_length += 1;
		}else{
			// copy the word so far and continue to next word
			if(this_part_length != 0){
				ret_str_array[word_read_count] = calloc(this_part_length + 1, sizeof(char));
				strncpy(ret_str_array[word_read_count], traverser - this_part_length, this_part_length);
				word_read_count += 1;
			}
			this_part_length = 0;
		}
		traverser++;
	}
	// the last word
	if(this_part_length != 0){
		ret_str_array[word_read_count] = calloc(this_part_length + 1, sizeof(char));
		strncpy(ret_str_array[word_read_count], traverser - this_part_length, this_part_length);
		word_read_count += 1;
	}
	*num_token = word_read_count;
	return ret_str_array;
}

// free the result of split args
void free_args(char **args){
	char** traverser = args;
	while(traverser[0] != NULL){
		free(traverser[0]);
		traverser++;
	}
	free(args);
}

// int main(){
// 	char** ch = strsplit("  mother  fucker shit  ", 3);
// 	for(int i = 0; i < 3; i++){
// 		printf("%s\n", ch[i]);
// 	}
// 	free_args(ch);
// }

// handle read & write
ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    printf("count: %lu\n", count);
    ssize_t total_read = 0;
    size_t total_request = count;
    while(1){
        ssize_t this_read = read(socket, buffer, count);
        // read something
        if(this_read > 0){
            total_read += this_read;
            buffer += this_read;
            count -= this_read;
        }
        // succeeds
        if((size_t) total_read == total_request){
            return total_read;
        }
        // fails
        if(this_read == 0){
            return total_read;
        }else if(this_read == -1){
            if(errno == EINTR){
                continue;
            }else{
                return -1;
            }
        }
        // only partially done
    }
}

ssize_t write_all_to_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    ssize_t total_write = 0;
    size_t total_request = count;
    while(1){
        ssize_t this_write = write(socket, buffer, count);
        // read something
        if(this_write > 0){
            total_write += this_write;
            buffer += this_write;
            count -= this_write;
        }
        // succeeds
        if((size_t) total_write == total_request){
            return total_write;
        }
        // fails
        if(this_write == 0){
            return total_write;
        }else if(this_write == -1){
            if(errno == EINTR){
                continue;
            }else{
                return -1;
            }
        }
        // only partially done
    }
}
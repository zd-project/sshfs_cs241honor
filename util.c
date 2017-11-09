#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
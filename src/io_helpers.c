#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#include "io_helpers.h"
#include "linked_list.h"

// ===== Input helpers =====
/* Prereq: in_ptr is a null terminated string
 * Return: 0 if in_ptr is a positive integer, 1 if in_ptr is not a positive integer
 */
int is_pos_num(char *in_ptr) {
	int retval = 0;
	int len = strlen(in_ptr);
	for(int i = 0; i < len; i++) {
		if(isdigit(in_ptr[i]) == 0) {
				retval = 1;
		}
	}
	return retval;
}

/* find_equals finds an equals sign in the given token
 * prereq: in_ptr is not NULL
 * return: index of equals sign if found, -1 if no equal sign found, -2 if in_ptr is NULL
 */
int find_equals(char *in_ptr) {
        if(in_ptr == NULL) {
                display_error("ERROR: ", "in_ptr is NULL, aborting find_equals.");
                return -2;
        }

        int len = strlen(in_ptr);
        int i;
        for(i = 0; i < len; i++) {
                if(in_ptr[i] == '=') {
                        return i;
                }
        }
        return -1;
}

/* find_bg checks if a & sign is present at any location in the given tokens array
 * prereq: tokens is not NULL
 * return: -1 if no & is present, the last tokens index if a & is located at the end of the last token, -2 if there are several &'s or it is not located at the front.
 */
int find_bg(char **tokens) {
	int num_bg = 0;
	int t_index = 0;
	int index = 0;
	while(tokens[t_index] != NULL) {
		if(tokens[t_index][index] == '&'){
			if(tokens[t_index + 1] == NULL && tokens[t_index][index + 1] == '\0' && num_bg == 0) {
				num_bg++;
				return t_index;
			} else {
				num_bg++;
				index++;
			}
		} else {
			if(tokens[t_index][index] == '\0') {
				t_index++;
				index = 0;
			} else {
				index++;
			}
		}
	}
	if(num_bg == 0) {
		return -1;
	} else {
		// display_error("ERROR: Too many & signs / & sign not at end of command", "");
		return -1;
	}
}

/* find_pipe finds a pipe sign in the given tokens array
 * prereq: tokens is not NULL
 * return: Array of token:index pairs where each pipe was found, NULL if no pipe was found
 */
int *find_pipe(char **tokens) {
        if(tokens == NULL) {
                display_error("ERROR: ", "in_ptr is NULL, aborting find_pipe.");
                return NULL;
        }
	
	int num_found = 0;
	int i = 0;
	int t_len;
	int t_index;
	int *pipes;
	while(tokens[i] != NULL) {
		t_len = strlen(tokens[i]);
		for(t_index = 0; t_index < t_len; t_index++) {
			if(tokens[i][t_index] == '|') {
				num_found++;
			}
		}
		i++;
	}
	if(num_found == 0) {
		return NULL;
	} else {
		pipes = malloc(sizeof(int) * (num_found * 2 + 3));;
		pipes[0] = num_found;
		pipes[1] = 0;
		pipes[2] = 0;
		i = 0;
		num_found = 3;
		while(tokens[i] != NULL) {
                	t_len = strlen(tokens[i]);
                	for(t_index = 0; t_index < t_len; t_index++) {
                        	if(tokens[i][t_index] == '|') {
                                	pipes[num_found] = i;
					pipes[num_found + 1] = t_index;
					num_found += 2;
				}
                        }
			i++;
                }
	}
	return pipes;
}

// Don't use this, this has no other uses besides what i'm doing with this.
int build_token_arr(char **token_arr, char ***new_token_arr, int token1, int index1) {
	if(token1 == index1 && token1 == 0) {
                if(token_arr[token1][index1] == '|') {
                        display_error("ERROR: Pipe operator used without command", "");
                        return -1;
                }
        }
	(*new_token_arr) = malloc(sizeof(char *) * 128);
	if(*new_token_arr == NULL) {
		perror("malloc");
		return -1;
	}
	int t_index = 0;
	int index = 0;
	int retval = 0;
	if(token_arr[token1][index1] == '|') {
		if(token_arr[token1][index1 + 1] != '\0') {
			index1++;
		} else {
			if(token_arr[token1 + 1] != NULL) {
				token1++;
				index1 = 0;
			} else {
				display_error("ERROR: Pipe operator used without command", "");
				free((*new_token_arr));
				return -1;
			}
		}
	}
	while(token_arr[token1] != NULL) {
		(*new_token_arr)[t_index] = malloc(sizeof(char) * MAX_STR_LEN);
		retval++;
                if((*new_token_arr)[t_index] == NULL) {
                	perror("malloc");
                        return -1;
                }
		index = 0;
		while(token_arr[token1][index1] != '\0' && token_arr[token1][index1] != '|') {
			(*new_token_arr)[t_index][index] = token_arr[token1][index1];
			index++;
			index1++;
		}

		if(token_arr[token1][index1] == '|') {
			(*new_token_arr)[t_index][index] = '\0';
			(*new_token_arr)[t_index + 1] = NULL;
			retval++;
			return retval;
		} else if(token_arr[token1][index1] == '\0') {
			if(token_arr[token1 + 1] != NULL) {
				if(token_arr[token1 + 1][0] != '|') {
					(*new_token_arr)[t_index][index] = '\0';
					token1++;
					index1 = 0;
					t_index++;
					index = 0;
				} else {
					(*new_token_arr)[t_index][index] = '\0';
					(*new_token_arr)[t_index + 1] = NULL;
					retval++;
					return retval;
				}
			} else {
				(*new_token_arr)[t_index][index] = '\0';
				(*new_token_arr)[t_index + 1] = NULL;
				retval++;
				return retval;
			}
		}
	}
	(*new_token_arr)[t_index + 1] = NULL;
	retval++;
	return retval;
}

/* Expands any "..." and "...." elements in <path>, places expanded path in <buffer>
 * Prereq: <path> is null-terminated
 * Return: None
 */
void expand_path(char *path, char *buffer) {
	int path_pos = 0;
	while(path[path_pos] != '\0') {
		if(path[path_pos] == '.' && path[path_pos + 1] == '.' && path[path_pos + 2] == '.' && path[path_pos + 3] == '.') {
			strncat(buffer, "../../..", 9);
			path_pos = path_pos + 4;
		} else if(path[path_pos] == '.' && path[path_pos + 1] == '.' && path[path_pos + 2] == '.') {
			strncat(buffer, "../..", 6);
			path_pos = path_pos + 3;
		} else {
			strncat(buffer, &path[path_pos], 1);
			path_pos++;
		}
	}
}

/* Gets all flag info for ls and places their values into their respective variable.
 * prereq: <tokens> is an array of null-terminated strings, <f_substring> and <path> are of size MAX_STR_LEN
 * return: 0 if successful, -1 if an error is encountered
 */ 
int ls_get_flags(char ** tokens, int *d, int *rec, char *f_substring, char *path) {
        char f_check[MAX_STR_LEN];
	memset(f_check, '\0', MAX_STR_LEN);
        int rec_check = 0;
        int d_check = -1;
        char path_string[MAX_STR_LEN];
	memset(path_string, '\0', MAX_STR_LEN);
        int index = 1;
        while(tokens[index] != NULL) {
                if((strncmp("--f", tokens[index], 4) == 0) && (tokens[index + 1] != NULL)) {
                        if((strncmp("-", tokens[index + 1], 1) != 0) && f_check[0] == '\0') {
                                strcpy(f_check, tokens[index + 1]);
                                index = index + 2;

                        } else if((strncmp("-", tokens[index + 1], 1) != 0) && (strcmp(f_check, tokens[index + 1]) != 0)) {
        			display_error("ERROR: ", "All uses of --f must be used with the same substring");
                                return -1;

                        } else if(strncmp("-", tokens[index + 1], 1) == 0) {
                              	display_error("ERROR: ", "--f takes a substring that DOES NOT start with '-'");
                                return -1;

                        } else {
                                index = index + 2;
                        }

		 } else if(strcmp("--f", tokens[index]) == 0) {
			display_error("ERROR: ", "--f used without substring.");
                        return -1;

                } else if(strcmp("--rec", tokens[index]) == 0) {
                        rec_check = 1;
                        index++;

                } else if((strcmp("--d", tokens[index]) == 0) && (tokens[index + 1] != NULL)) {
                        if((is_pos_num(tokens[index + 1]) == 0) && d_check == -1) {
                                d_check = strtol(tokens[index + 1], NULL, 10);
                                index = index + 2;
                        } else if((is_pos_num(tokens[index + 1]) == 0) && (strtol(tokens[index + 1], NULL, 10) == d_check))  {
                                index = index + 2;
                        } else if(is_pos_num(tokens[index + 1]) == 0) {
				display_error("ERROR: ", "All uses of --d must be used with the same value");
                                return -1;
                        } else {
				display_error("ERROR: ", "--d takes a positive number argument");
                                return -1;
                        }

                } else if(strcmp("--d", tokens[index]) == 0) {
			display_error("ERROR: ", "--d takes a positive number argument");
                        return -1;

                } else {
                        if(path[0] != '\0') {
				display_error("ERROR: ", "Too many arguments: ls takes a single path");
                                return -1;
                        }
                        strncpy(path_string, tokens[index], 128);
                        expand_path(path_string, path);
                        index++;
                }



        }
	if(path[0] == '\0') {
		strncpy(path, ".", 2);
	}
        // printf("f_check: %s, d_check: %d, rec_check: %d, path: %s\n", f_check, d_check, rec_check, path);
	*d = d_check;
	strncpy(f_substring, f_check, MAX_STR_LEN);
	*rec = rec_check;
        return 0;
}


// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
}


/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(char *pre_str, char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}

/* Collects and prints done messages for all background processes that have exited
 * prereq: in_ptr is not NULL
 * returns: number of messages collected
 */
int collect_done_messages(Node *in_ptr) {
	char *done_message;
	int retval = 0;
	Node *prev = in_ptr;
	Node *curr = in_ptr->next;
	Node *temp;
	while(curr != NULL) {
		if(strtol(curr->value, NULL, 10) < 0) {
			done_message = malloc(sizeof(char) * (22 + strlen(curr->name)));
			memset(done_message, '\0', 22 + strlen(curr->name));
                	sprintf(done_message, "[%ld]+  Done %s\n", (strtol(curr->value, NULL, 10) * -1), curr->name);
                	display_message(done_message);
			free(done_message);
			free(curr->value);
			free(curr->name);
			temp = curr;
			curr = curr->next;
			free(temp);
			prev->next = curr;
			retval++;
		} else {
			prev = prev->next;
			curr = curr->next;
		}
	}
	return retval;
}




/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1); // Not a sanitizer issue since in_ptr is allocated as MAX_STR_LEN+1
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }

    // printf("MAX_STR_LEN: %d, read_len: %d\n", MAX_STR_LEN, read_len);
    if (read_len > MAX_STR_LEN) {
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
	int junk = 0;
	char check = in_ptr[MAX_STR_LEN];
	if(check != EOF && check != '\n') {
             while((junk = getchar()) != EOF && junk != '\n');
	}
    }
    in_ptr[read_len] = '\0';
    return retval;
}

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens, -1 if an error occurs.
 */
int tokenize_input(char *in_ptr, char **tokens) {
    if(in_ptr == NULL || tokens == NULL) {
	    display_error("ERROR: ", "in_ptr or tokens is NULL, aborting tokenize_input.");
	    return -1;
    }

    char *curr_ptr = strtok (in_ptr, DELIMITERS);
    int token_count = 0;
    int chars_written = -1; // starts at -1 since we add 1 at the beginning of every loop to account for spaces in the expanded line
    char out_ptr[MAX_STR_LEN];
    int out_ptr_len;

    while (curr_ptr != NULL && token_count < MAX_STR_LEN) {
	chars_written++;
	expand(curr_ptr, out_ptr, &chars_written);
	out_ptr_len = strlen(out_ptr);
        tokens[token_count] = malloc(sizeof(char) * out_ptr_len + 1);

	if(tokens[token_count] == NULL) {
		display_error("ERROR: ", "Allocation of space for token failed, aborting tokenize_input.");
		for(int i = 0; i <= token_count; i++) {
			free(tokens[i]);
		}
		return -1;
	}

	strncpy(tokens[token_count], out_ptr, out_ptr_len + 1);
	// printf("tokens[%d]: %s\n", token_count, tokens[token_count]);
        token_count++;
	curr_ptr = strtok(NULL, DELIMITERS);
	memset(out_ptr, '\0', MAX_STR_LEN);
    }
    tokens[token_count] = NULL;
    return token_count;
}

/* freeArr frees all non-NULL entries in arr
 * prereq: arr is an array of dynamically allocated pointers
 * return: entries freed or -1 if an error occurs
 */
int freeArr(char **arr) {
	if(arr == NULL) {
		display_error("ERROR: ", "arr is NULL, aborting freeArr.");
		return -1;
	}
	int i = 0;
	int retval = 0;
	while(arr[i] != NULL) {
		free(arr[i]);
		arr[i] = NULL;
		retval++;
		i++;
	}
	return retval;
}


/* expands all variables in in_ptr and places the expanded output into out_ptr until *chars_written = MAX_STR_LEN, at which point it null terminates the current token
 * prereq: in_ptr is null-terminated and not NULL, out_ptr is not NULL
 * return: total characters written during this tokenization
 */
void expand(char *in_ptr, char *out_ptr, int *chars_written) {
	if(*chars_written >= MAX_STR_LEN - 1) {
		return;
	}
	int in_pos = 0;
	int out_pos = 0;
	int var_pos = 0;
	int increment = 1;
	char next;
	char var_name[MAX_STR_LEN + 1];
	int var_length = 0;
	char *var_value;

	while(in_ptr[in_pos] != '\0' && *chars_written != MAX_STR_LEN - 1) {
		if(in_ptr[in_pos] == '$' && front != NULL) {
			next = in_ptr[in_pos + increment];
			if(next == ' ' || next == '\n' || next =='\t' || next == '\0' || next == '$') {
				out_ptr[out_pos] = in_ptr[in_pos];
				out_pos++;
				in_pos++;
				(*chars_written)++;
			} else {
				
				while(next != ' ' && next != '\n' && next != '\t' && next != '\0' && next != '$') {
					var_name[var_length] = next;
					var_length++;
					increment++;
					next = in_ptr[in_pos + increment];
				}
				var_name[var_length] = '\0';
				var_value = findValue(var_name, front);
				while(*chars_written < MAX_STR_LEN - 1 && var_value[var_pos] != '\0') {
					out_ptr[out_pos] = var_value[var_pos];
					out_pos++;
					var_pos++;
					(*chars_written)++;
				}
				in_pos += var_length + 1;
				increment = 1;
				var_length = 0;
				memset(var_name, '\0', MAX_STR_LEN);
				var_pos = 0;
			}
		} else {
			out_ptr[out_pos] = in_ptr[in_pos];
			out_pos++;
			in_pos++;
			(*chars_written)++;
		}
	}
	out_ptr[*chars_written] = '\0';
	// printf("out_ptr: %s\n", out_ptr);
}

/* Recursive ls helper function
 * prereq: don't use this
 * return: idk, 0 if successful, -1 if it fails lol
 */
int ls_recursive(char *filepath, int depth, char *f_substring, int out_fd) {
	(void)out_fd;
	
	if(depth == 0) {
		return 0;
	}
	DIR *path;
	path = opendir(filepath);
	// fprintf(stderr, "%s\n", filepath);
	if(path == NULL) {
		display_error("ERROR: ", "Invalid path");
		return -1;
	}
	
	struct dirent *file;
	char *newFilepath;
	int errorCheck = 0;
	char *filename;
	if(f_substring[0] != '\0') {
		while((file = readdir(path)) != NULL) {
			if(strstr(file->d_name, f_substring) != NULL) {
				filename = malloc(sizeof(char) * strlen(file->d_name) + 2);
				strcpy(filename, file->d_name);
				strcat(filename, "\n");
				display_message(filename);
				
				free(filename);
			}
			if(file->d_type == DT_DIR && ((strcmp(".", file->d_name) != 0) && strcmp("..", file->d_name) != 0) ) {
				newFilepath = malloc(sizeof(char) * (strlen(filepath) + strlen(file->d_name) + 2));
				if(newFilepath == NULL) {
					display_error("ERROR: ", "memory allocation failed in recursive ls call");
					return -1;
				}
				strncpy(newFilepath, filepath, strlen(filepath) + 1);
				strncat(newFilepath, "/", 2);
				strncat(newFilepath, file->d_name, strlen(file->d_name));
				errorCheck = ls_recursive(newFilepath, depth - 1, f_substring, out_fd);
				free(newFilepath);
				if(errorCheck == -1) {
					display_error("ERROR: ", "recursive ls call failed");
					return -1;
				}
			}
		}
	} else {
		while((file = readdir(path)) != NULL) {
                	filename = malloc(sizeof(char) * strlen(file->d_name) + 2);
                        strcpy(filename, file->d_name);
                        strcat(filename, "\n");
                        display_message(filename);
                        free(filename);
                        if(file->d_type == DT_DIR && ((strcmp(".", file->d_name) != 0) && strcmp("..", file->d_name) != 0) ) {
                        	newFilepath = malloc(sizeof(char) * (strlen(filepath) + strlen(file->d_name) + 2));
                                if(newFilepath == NULL) {
                                	display_error("ERROR: ", "memory allocation failed in recursive ls call");
                                        return -1;
                                }
                                strncpy(newFilepath, filepath, strlen(filepath) + 1);
				strncat(newFilepath, "/", 2);
                                strncat(newFilepath, file->d_name, strlen(file->d_name));
                                errorCheck = ls_recursive(newFilepath, depth - 1, f_substring, out_fd);
                                free(newFilepath);
                                if(errorCheck == -1) {
                                	display_error("ERROR: ", "recursive ls call failed");

                                        return -1;
				}
			}
		}
	}
	closedir(path);
	return 0;
}

/* tokens_to_string takes in an array of NULL-terminated strings and returns a pointer
 * to a dynamically allocated string of all the tokens, each with a space separating 
 * them
 * prereq: All strings in tokens are NULL-terminated
 * return: Dynamically allocated string of all tokens, each with a space separating
 * them
 */
char *tokens_to_string(char **tokens) {
	if(tokens == NULL || tokens[0] == NULL) {
		char *retval = malloc(sizeof(char));
		retval[0] = '\0';
		return retval;
	}

	int total_len = 0;
	int i = 0;
	while(tokens[i] != NULL) {
		total_len += strlen(tokens[i]);
		total_len++;
		i++;
	}
	char *retval = malloc(sizeof(char) * (total_len + 1));
	retval[0] = '\0';
	i = 0;
	while(tokens[i] != NULL) {
		strcat(retval, tokens[i]);
		if(tokens[i + 1] != NULL) {
			strcat(retval, " ");
		}
		i++;
	}
	return retval;
}


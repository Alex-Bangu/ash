#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "builtins.h"
#include "io_helpers.h"


// ====== Command execution =====

/* Return: pointer of a function to handle builtin or NULL if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    if(cmd_num < BUILTINS_COUNT) {
	    return(BUILTINS_FN[cmd_num]);
    }
    char path[strlen(cmd) + 10];
    strncpy(path, "/bin/", 10);
    strncat(path, cmd, strlen(cmd) + 1);
    if(access(path, F_OK) == 0 || access(cmd, F_OK) == 0) {
	    return(BUILTINS_FN[cmd_num]);
    } else {
	    return NULL;
    }
}


// ===== Builtins =====

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_echo(char **tokens, int in_fd, int out_fd) {
    (void)in_fd;
    ssize_t index = 1;

    if(out_fd == -1) {
	    (void)out_fd;
    }

    if (tokens[index] != NULL) {
	display_message(tokens[index]);
	index++;
    }
    while (tokens[index] != NULL) {
	display_message(" ");
	display_message(tokens[index]);
	index++;
    }
    display_message("\n");
    return 0;
}

/* prereq: tokens is a null terminated sequence of strings
 * return: 0 on success and -1 on error
 */
ssize_t bn_ls(char **tokens, int in_fd, int out_fd) {
	(void)in_fd;
	int d;
	char f_substring[MAX_STR_LEN];
	memset(f_substring, '\0', MAX_STR_LEN);
	int rec;
	char path[MAX_STR_LEN];
	memset(path, '\0', MAX_STR_LEN);
	int err_check = ls_get_flags(tokens, &d, &rec, f_substring, path);
	if(err_check == -1) {
		return -1;
	}

	if(d != -1 && rec == 0) {
		display_error("ERROR: ", "--d must be used with --rec");
		return -1;
	
	} else if(rec == 1 && d == -1) {
		err_check = ls_recursive(path, -1, f_substring, out_fd);
		if(err_check == -1) {
			return -1;
		}

	} else if(rec == 0 && d == -1) {
		err_check = ls_recursive(path, 1, f_substring, out_fd);
		if(err_check == -1) {
			return -1;
		}
	} else {
		err_check = ls_recursive(path, d, f_substring, out_fd);
		if(err_check == -1) {
			return -1;
		}
	}
	return 0;
}

/* cd builtin, changes the current working directory
 * prereq: tokens is a null terminated sequence of strings
 * return: 0 if successful, -1 if an error occurs
 */
ssize_t bn_cd(char **tokens, int in_fd, int out_fd) {
	char path_buffer[MAX_STR_LEN];
	memset(path_buffer, '\0', MAX_STR_LEN);
	int chdir_check;
	(void)in_fd;
	(void)out_fd;

	if(tokens[2] != NULL) {
		display_error("ERROR: ", "Too many arguments: cd takes a single path");
		return -1;
	} else if(tokens[1] == NULL) {
		display_error("ERROR: No path provided", "");
		return -1;
	} else {
		// fprintf(stderr, "path_buffer: %s\n", path_buffer);
		expand_path(tokens[1], path_buffer);
		// fprintf(stderr, "%s\n", path_buffer);
		chdir_check = chdir(path_buffer);
		if(chdir_check == -1) {
			display_error("ERROR: ", "Invalid path");
			return -1;
		} else {
			return 0;
		}
	}

}

/* cat builtin, prints all file contents to stdout
 * prereq: tokens is a sequence of null terminated strings
 * return: 0 if successful, -1 if an error occurs
 */
ssize_t bn_cat(char **tokens, int in_fd, int out_fd) {
	(void)out_fd;
	if(tokens[1] == NULL) {
		if(in_fd == -1) {
			display_error("ERROR: No input source provided", "");
			return -1;
		} else {
			char temp[2];
			char *fbuf;
			fbuf = malloc(sizeof(char) * 2);
			if(fbuf == NULL) {
				perror("malloc");
				return -1;
			}
			int fbuf_size = 2;
			memset(temp, '\0', 2);
			memset(fbuf, '\0', 2);
                        int nbytes;
                        while((nbytes = read(in_fd, temp, 1)) > 0) {
				fbuf_size += nbytes;
				fbuf = realloc(fbuf, sizeof(char) * fbuf_size);
				if(fbuf == NULL) {
					perror("realloc");
					return -1;
				}
				temp[1] = '\0';
				strncat(fbuf, temp, 1);
				memset(temp, '\0', 2);
			}
			if(nbytes == 0) {
				display_message(fbuf);
				free(fbuf);
				return 0;
			} else {
				display_error("ERROR: Failed to read from input source", "");
				free(fbuf);
				return -1;
			}
		}

	} else if(tokens[2] != NULL) {
		display_error("ERROR: ", "Too many arguments: cat takes a single file");
		return -1;
	}
	struct stat file_stat;
    	if (stat(tokens[1], &file_stat) == -1) {
        	display_error("ERROR: Cannot open file", "");
        	return -1;
    	}

    	if (S_ISDIR(file_stat.st_mode)) {
        	display_error("ERROR: ", "Cannot open file");
        	return -1;
    	}

	FILE *file;
	char buffer[MAX_STR_LEN];
	memset(buffer, '\0', MAX_STR_LEN);
	int bytesRead;
	file = fopen(tokens[1], "r");
	if(file == NULL) {
		display_error("ERROR: ", "Cannot open file");
		return -1;
	}
	bytesRead = fread(buffer, sizeof(char), MAX_STR_LEN - 1, file);
	while(bytesRead != 0) {
		display_message(buffer);
		memset(buffer, '\0', MAX_STR_LEN);
		bytesRead = fread(buffer, sizeof(char), MAX_STR_LEN - 1, file);
	}
	fclose(file);
	return 0;
}

/* takes in a file path and outputs the word count, character count, and new line count, to stdout
 * prereq: tokens is a sequence of null terminated strings
 * return: 0 if successful, -1 if an error occured
 */
ssize_t bn_wc(char **tokens, int in_fd, int out_fd) {
	FILE *file;
        int c;
        int c_count = 0;
        int w_count = 0;
        int nl_count = 0;
        int last_char_was_whitespace = 0;
        int is_first_word = 1;
        char int_buffer[MAX_STR_LEN];
	if(out_fd !=- -1) {
		dup2(out_fd, STDOUT_FILENO);
	}
	if(tokens[1] == NULL) {
                if(in_fd == -1) {
			display_error("ERROR: No input source provided", "");
			return -1;
		}
		file = fdopen(in_fd, "r");

        } else {
		if(tokens[2] != NULL) {
                	display_error("ERROR: ", "Too many arguments: wc takes a single file");
			return -1;
        	}
        	struct stat file_stat;
        	if (stat(tokens[1], &file_stat) == -1) {
                	perror("stat");
                	return -1;
        	}

        	if (S_ISDIR(file_stat.st_mode)) {
                	display_error("ERROR: ", "Cannot open file");
         		return -1;
        	}
	
		file = fopen(tokens[1], "r");
	}

	if(file == NULL) {
		display_error("ERROR: ", "Cannot open file");
	}
	while((c = fgetc(file)) != EOF) {
		c_count++;
		if(is_first_word == 1 && !(isspace(c))) {
			w_count++;
			is_first_word = 0;
		} else if(last_char_was_whitespace == 1 && !(isspace(c))) {
			w_count++;
			last_char_was_whitespace = 0;
		} else if(isspace(c)) {
			last_char_was_whitespace = 1;
		}
		if(c == '\n') {
			nl_count++;
		}
	}

	
	fclose(file);

	display_message("word count ");
	sprintf(int_buffer, "%d\n", w_count);
	display_message(int_buffer);	
	
	display_message("character count ");
	sprintf(int_buffer, "%d\n", c_count);
	display_message(int_buffer);

	display_message("newline count ");
	sprintf(int_buffer, "%d\n", nl_count);
	display_message(int_buffer);

	return 0;
}

/* Finds and executes the command in tokens[0] by looking in /bin or /usr/bin
 * prereq: tokens is a sequence of null terminated strings
 * return: 0 if successful, -1 if an error occured
 */
ssize_t bn_bin(char **tokens, int in_fd, int out_fd) {
	if(*tokens[0] == '\0') {
		return 0;
	}
	(void)in_fd;
	(void)out_fd;
	int n;
	int exec_check;
	if(to_exec == 0) {

		n = fork();
		if(n < 0) {
			perror("fork");
			return -1;
		} else if(n == 0) {
			exec_check = execvp(tokens[0], tokens);
			if(exec_check == -1) {
				perror("execvp");
				exit(-1);
			}
		} else {
			waitpid(n, NULL, 0);
		}
	} else {
		exec_check = execvp(tokens[0], tokens);
		if(exec_check == -1) {
			perror("execvp");
			return -1;
		}
	}
	return 0;
}

ssize_t bn_start_server(char **tokens, int in_fd, int out_fd) {
	if(server_pid != -1) {
		display_error("ERROR: Server already running", "");
		return -1;
	}
	(void)in_fd;
        (void)out_fd;
        int n;
        int exec_check;
	free(tokens[0]);
        tokens[0] = malloc(sizeof(char) * 14);
        strncpy(tokens[0], "./chat_server", 14);
        if(tokens[1] == NULL) {
                display_error("ERROR: No port provided", "");
               	return -1;
        }
	n = fork();
	if(n == 0) {
		int fd = dup(STDOUT_FILENO);
		if (fd == -1) {
    			perror("dup");
    			return -1;
		}
		dup2(fd, STDOUT_FILENO);
		close(fd);
		fflush(stdout);
		exec_check = execvp(tokens[0], tokens);
		if(exec_check == -1) {
			perror("execvp");
			exit(-1);
		}
	} else if(n < 0) {
		perror("fork");
		return -1;
	} else if(n > 0) {
		server_pid = n;
	}
	return 0;
}

ssize_t bn_close_server(char **tokens, int in_fd, int out_fd) {
	(void)in_fd;
	(void)out_fd;
	(void)tokens;
        if(server_pid != -1) {
               kill(server_pid, SIGINT);
	       server_pid = -1;
	       return 0;	
        } else {
		display_error("ERROR: No server is currently running", "");
		return -1;
	}
}


ssize_t bn_start_client(char **tokens, int in_fd, int out_fd) {
	if(tokens[1] == NULL) {
		display_error("ERROR: No port provided", "");
		return -1;
	} else if(tokens[2] == NULL) {
		display_error("ERROR: No hostname provided", "");
		return -1;
	}
	(void)in_fd;
        (void)out_fd;
        int n;
        int exec_check;
        free(tokens[0]);
        tokens[0] = malloc(sizeof(char) * 14);
        strncpy(tokens[0], "./chat_client", 14);
	n = fork();
	if(n == 0) {
		int fd = dup(STDOUT_FILENO);
                if (fd == -1) {
                        perror("dup");
                        return -1;
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                fflush(stdout);
		exec_check = execvp(tokens[0], tokens);
		if(exec_check == 1) {
			perror("exec");
			return -1;
		}
	} else if(n > 0) {
		waitpid(n, NULL, 0);
	} else {
		perror("fork");
		return -1;
	}
	return 0;
}

ssize_t bn_send(char **tokens, int in_fd, int out_fd) {
        if(tokens[1] == NULL) {
                display_error("ERROR: No port provided", "");
                return -1;
        } else if(tokens[2] == NULL) {
                display_error("ERROR: No hostname provided", "");
                return -1;
        } else if(tokens[3] == NULL) {
		display_error("ERROR: No message provided", "");
		return -1;
	}
        (void)in_fd;
        (void)out_fd;
        int n;
        int exec_check;
	char *msg;
	msg = tokens_to_string(&tokens[3]);
	free(tokens[3]);
	tokens[3] = msg;
        free(tokens[0]);
        tokens[0] = malloc(sizeof(char) * 7);
        strncpy(tokens[0], "./send", 7);
        n = fork();
        if(n == 0) {
                exec_check = execvp(tokens[0], tokens);
                if(exec_check == 1) {
                        perror("exec");
                        return -1;
                }
        } else if(n < 0) {
                perror("fork");
                return -1;
        } else {
		waitpid(n, NULL, 0);
	}
        return 0;
}




ssize_t bn_exit(char **tokens, int in_fd, int out_fd) {
	(void)tokens;
	(void)in_fd;
	(void)out_fd;
	return 0;
}
ssize_t bn_kill(char **tokens, int in_fd, int out_fd) {
	(void)in_fd;
	(void)out_fd;
	int signo;
	int pid;
	if(tokens[1] == NULL) {
		display_error("ERROR: pid not provided\n", "");
		return -1;
	}

	pid = strtol(tokens[1], NULL, 10);

	if(tokens[2] == NULL) {
		signo = SIGTERM;
	} else {
		signo = strtol(tokens[2], NULL, 10);
	}

	struct sigaction sa;
    if (sigaction(signo, NULL, &sa) != 0 || errno == EINVAL) {
	display_error("ERROR: Invalid signal specified", "");
	return -1;
    }
    if(kill(pid, 0) != 0) {
	display_error("ERROR: The process does not exist", "");
	return -1;
    }

    if(kill(pid, signo) == 0) {
	    return 0;
    } else {
	    perror("kill");
	    return -1;
    }
    return 0;
    
}

ssize_t bn_ps(char **tokens, int in_fd, int out_fd) {
	(void)tokens;
	(void)in_fd;
	(void)out_fd;
	Node *front = command_nums;
	front = front->next;
	while(front != NULL) {
		char buf[strlen(front->name) + 20];
		sprintf(buf, "%s %ld\n", front->name, front->pid);
		display_message(buf);
		front = front->next;
	}
	return 0;
}

	



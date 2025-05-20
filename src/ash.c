#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>


#include "builtins.h"
#include "io_helpers.h"
#include "linked_list.h"

Node dummy = {"", "", NULL, -1};
Node *front = &dummy;
Node *command_nums = &dummy;

int bg_exited = 0;
int done = 0;
int to_exec = 0;
int server_pid = -1;
int exiting = 0;

// sigaction handler
void handler(int signo, siginfo_t *si, __attribute__((unused)) void *ucontext) {
	if(exiting) {
		return;
	}
	if(signo == SIGCHLD) {
		Node *curr = command_nums->next;
		while(curr != NULL) {
				if(curr->pid == si->si_pid) {
					done = 0;
                			bg_exited++;
					sprintf(curr->value, "%ld", (strtol(curr->value, NULL, 10) * -1));
					return;
				} else {
					curr = curr->next;
				}
		}
	} else if(signo == SIGINT) {
		display_message("\n");
		return;
	}
}

int main(__attribute__((unused)) int argc, 
         __attribute__((unused)) char* argv[]) {

    // setting up signal handler
    struct sigaction sa;
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
	
    // Shell prompt
    char prompt[1024];

    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};
    
    // Setting up stuff for pipes
    char **new_token_arr;
    int *pipes;

    // child catching
    int n = -1;
    int total_children;
    int num_children = 0;

    // background stuff
    int bg_fork = -1;

    // Initializing a dummy variable node that holds the empty string
    char *empty = malloc(sizeof(char) * 2);
    strncpy(empty, "", 2);
    char *end_str = malloc(sizeof(char) * 2);
    strncpy(end_str, "", 2);
    front = makeNode(end_str, empty);

    // Setting up  background process linked list
    int num_commands = 0;
    char *name = malloc(sizeof(char) * 5);
    strcpy(name, "ash");
    char *value = malloc(sizeof(char) * 5);
    strcpy(value, "0");
    command_nums = makeNode(name, value);
    command_nums->pid = getpid();
    Node *curr_c = command_nums;

    char *pname = malloc(sizeof(char) * 5);
    strcpy(pname, "ash");

    while (1) {
	getcwd(prompt, 1024);
	strncat(prompt, "$ ", 3);
    	// strncat(prompt, ":ash$ ", 8);
	// hunting children
	if(n == 0) {
		freeLinkedList(front);
            	freeArr(token_arr);
		exit(1);
	}
	total_children = 0;
	n = -1;
	if(bg_fork == 0) {
		free(pname);
		freeLinkedList(front);
		freeArr(token_arr);
		freeLinkedList(command_nums);
		exit(1);
	}

	bg_fork = -1;
        // Prompt and input tokenization
	
	// Collect done messages
        if(bg_exited != 0) {
		done = 1; 
	} else {
		display_message(prompt);
	} 
	
	// Freeing token_arr contents
	freeArr(token_arr);
	int ret = get_input(input_buf);	
        size_t token_count = tokenize_input(input_buf, token_arr);

        // Clean exit

        if ((ret != -1 && token_count == 1 && strncmp("exit", token_arr[0], 4) == 0 && strlen(token_arr[0]) == 4 ) || ret == 0 || n == 0) {
	    exiting = 1;
	    freeLinkedList(front);
	    freeArr(token_arr);
	    freeLinkedList(command_nums);
	    free(pname);
	    if(server_pid != -1) {
		    kill(server_pid, SIGINT);
	    }
            return 0;
        }
	
	// need this here so we don't seg fault on empty input
	pipes = NULL;

       // Command execution
        if(bg_exited != 0 && done == 1) {
                collect_done_messages(command_nums);
                bg_exited = 0;
                done = 0;
	}

        if (token_count >= 1) {
	    int i = 0;
	    while((strncmp(token_arr[i], "", 2) == 0) && token_arr[i + 1] != NULL) {
		    i++;
	    }
	    // background check
	    int bg_check = find_bg(&token_arr[i]);
	    if(bg_check == -2) {
		    continue;
	    } else if(bg_check >= 0) {
		    curr_c = command_nums;
		    while(curr_c->next != NULL) {
			    curr_c = curr_c->next;
		    }
		    num_commands = strtol(curr_c->value, NULL, 10) + 1;
		    bg_fork = fork();
		    if(bg_fork == 0) {
			    free(pname);
			    pname = tokens_to_string(&token_arr[i]);
			    free(token_arr[i + bg_check]);
			    token_arr[i + bg_check] = NULL;
			    freeArr(&token_arr[i + bg_check + 1]);
			    char path[strlen(token_arr[i]) + 10];
                            strncpy(path, "/bin/", 10);
                            // I'm sorry for this next if condition, it's a new low for me (I hit a new low every single day.)
                            if((access(path, F_OK) == 0 || access(token_arr[i], F_OK) == 0) && ((strcmp(token_arr[i], "echo") != 0) && \
                                                        (strcmp(token_arr[i], "cat") != 0) && (strcmp(token_arr[i], "wc") != 0) && \
                                                        (strcmp(token_arr[i], "ls" ) != 0) && (strcmp(token_arr[i], "cd") != 0) && \
                                                        (strcmp(token_arr[i], "kill") != 0) && (strcmp(token_arr[i], "ps") != 0) && \
                                                        (strcmp(token_arr[i], "exit") != 0))) {
                                    to_exec++;
                            }

		    } else if(bg_fork > 0) {
			    Node *bg_prog = malloc(sizeof(Node));
			    bg_prog->name = tokens_to_string(&token_arr[i]);
			    bg_prog->value = malloc(sizeof(char) * 5);
			    bg_prog->pid = bg_fork;
			    sprintf(bg_prog->value, "%d", num_commands);
			    curr_c->next = bg_prog;
			    curr_c = curr_c->next;
			    curr_c->next = NULL;
			    char *done_msg;
			    done_msg = malloc(sizeof(char) * 32);
			    if(done_msg == NULL) {
				    free(done_msg);
				    perror("malloc");
				    continue;
			    }
			    memset(done_msg, '\0', 32);
			    sprintf(done_msg, "[%d]  %d\n", num_commands, bg_fork);
			    display_message(done_msg);
			    free(done_msg);
			    freeArr(&token_arr[i]);
			    continue;
		    } else {
			    perror("fork");
			    continue;
		    }
	    }
		    

	    // Pipe management
	    num_children = 0;
	    pipes = find_pipe(&token_arr[i]);
	    int (*pipe_fds)[2];
	    int my_pipe_r;
	    int my_pipe_w;
	    if(pipes != NULL) {
		total_children = pipes[0] + 1;
		int pipe_fds2[pipes[0]][2];
		// printf("num_pipes: %d\n", pipes[0]);
		pipe_fds = pipe_fds2;
		for(int p = 0; p < pipes[0]; p++) {
			pipe(pipe_fds[p]);
		}
	    	int y = 1;
	    	int arr_check;
	    	// Really rough code
	  
		    while((y < pipes[0] * 2 + 3) && n != 0) {
			    if((arr_check = build_token_arr(token_arr, &new_token_arr, pipes[y], pipes[y + 1])) == -1) {
				    // display_error("ERROR: build_token_arr() failed", "");
				    break;
			    }
			    num_children++;
			    n = fork();
			    if(n == 0) {
				    	// printf("tokens: %s, %s\n", new_token_arr[0], new_token_arr[1]);
					int x;
					freeArr(token_arr);
		            		for(x = 0; x < arr_check; x++) {
				    		token_arr[x] = new_token_arr[x];
			    		}
					
					// printf("tokens (again): %s, %s\n", new_token_arr[0], new_token_arr[1]);

					if(token_arr[x] != NULL) {
						token_arr[x] = NULL;
					}
					if(num_children == 1) { // first child
						dup2(pipe_fds[0][1], STDOUT_FILENO);
						my_pipe_w = pipe_fds[0][1];
						my_pipe_r = dup(STDIN_FILENO);
					} else if(num_children != total_children) { // not last child
						dup2(pipe_fds[num_children - 2][0], STDIN_FILENO);
						dup2(pipe_fds[num_children - 1][1], STDOUT_FILENO);
						my_pipe_r = pipe_fds[num_children - 2][0];
						my_pipe_w = pipe_fds[num_children - 1][1];
					} else { // last child
						dup2(pipe_fds[num_children - 2][0], STDIN_FILENO);
						my_pipe_r = pipe_fds[num_children - 2][0];
						// my_pipe_w = -1;
						my_pipe_w = dup(STDOUT_FILENO);
					}
					for(int pipe_num = 0; pipe_num < pipes[0]; pipe_num++) {
						if(pipe_fds[pipe_num][0] != my_pipe_r) {
							close(pipe_fds[pipe_num][0]);
							// fprintf(stderr, "closed read end of pipe %d\n", pipe_num);
						}
						if(pipe_fds[pipe_num][1] != my_pipe_w) {
							close(pipe_fds[pipe_num][1]);
							// fprintf(stderr, "closed write end of pipe %d\n", pipe_num);
						}
					}
					fflush(stdin);
					fflush(stdout);
			    } else if(n < 0) {
				    perror("fork");
				    return -1;
			    }
			    else {
				    freeArr(new_token_arr);
				    free(new_token_arr);
				    y += 2;
				    arr_check = 0;
			    }
		    }
		    if(n > 0) {
			for(int pipe_num = 0; pipe_num < pipes[0]; pipe_num++) {
                        	close(pipe_fds[pipe_num][0]);
                                close(pipe_fds[pipe_num][1]);
                        }
		    	free(pipes);
		    }
	    }
	    // End of really rough code (beginning of even more rough code)
	    if(n > 0) {
		    for(int c = 0; c < num_children; c++) {
			    wait(NULL);
		    }
		    continue;
	    }
            
            bn_ptr builtin_fn = check_builtin(token_arr[i]);
	    int var_check = find_equals(token_arr[i]);
	    ssize_t err;
            if (builtin_fn != NULL) {
		    if(n == 0) {
			    err = builtin_fn(token_arr, STDIN_FILENO, STDOUT_FILENO);
		    } else {
			    err = builtin_fn(token_arr, -1, -1);
		    }
            	    if (err == -1) {
                 	display_error("ERROR: Builtin failed: ", token_arr[i]);
            	    }
	    	    if(num_children == 1 && n == 0) { // first child
                                                close(pipe_fds[0][1]);
                                        } else if(num_children != total_children && n == 0) { // not last child
                                                close(pipe_fds[num_children - 1][0]);
                                                close(pipe_fds[num_children][1]);
                                        } else if(n == 0){ // last child
                                                close(pipe_fds[num_children - 2][0]);
                                        }	    

	    } else if(var_check != -1) {
		// printf("variable found\n"); 
           	// Initializing variable name
            	char *var_name = malloc(sizeof(char) * var_check + 1);
            	strncpy(var_name, token_arr[i], var_check);
            	var_name[var_check] = '\0';
            
            	// Initializing variable value
            	size_t var_size = sizeof(char) * (strlen(token_arr[i]) - var_check);
            	char *var = malloc(var_size + 1);
            	strncpy(var, &token_arr[i][var_check + 1], var_size);
            	var[var_size] = '\0';
	
	    	// Checking if variable is already initialized, if not, make new node
	    	int check = replaceNodeVal(front, var_name, var);
	    	if(check == 1) {
	    		Node *new_front = makeNode(var_name, var);
	    		new_front->next = front;
            		front = new_front;
			// printf("variable assigned\n");
	    	}

        } else {
           	display_error("ERROR: Unknown command: ", token_arr[i]);
        }
       
	}
	if(pipes != NULL) {
		free(pipes);
	}

    }

    return 0;
}

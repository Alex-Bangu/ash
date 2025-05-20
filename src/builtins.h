#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <unistd.h>

extern int to_exec;
extern int server_pid;

/* Type for builtin handling functions
 * Input: Array of tokens
 * Return: >=0 on success and -1 on error
 */
typedef ssize_t (*bn_ptr)(char **, int, int);
ssize_t bn_echo(char **tokens, int in_fd, int out_fd);
ssize_t bn_ls(char **tokens, int in_fd, int out_fd);
ssize_t bn_cd(char **tokens, int in_fd, int out_fd);
ssize_t bn_cat(char **tokens, int in_fd, int out_fd);
ssize_t bn_wc(char **tokens, int in_fd, int out_fd);
ssize_t bn_bin(char **tokens, int in_fd, int out_fd);
ssize_t bn_start_server(char **tokens, int in_fd, int out_fd);
ssize_t bn_close_server(char **tokens, int in_fd, int out_fd);
ssize_t bn_start_client(char **tokens, int in_fd, int out_fd);
ssize_t bn_send(char **tokens, int in_fd, int out_fd);
ssize_t bn_exit(char **tokens, int in_fd, int out_fd);
ssize_t bn_kill(char **tokens, int in_fd, int out_fd);
ssize_t bn_ps(char **tokens, int in_fd, int out_fd);


/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd);


/* BUILTINS and BUILTINS_FN are parallel arrays of length BUILTINS_COUNT
 */
static const char * const BUILTINS[] = {"echo_my", "ls_my", "cd", "cat_my", "wc_my", "exit", "kill_my", "ps_my", "start-server", "close-server", "start-client", "send"};
static const bn_ptr BUILTINS_FN[] = {bn_echo, bn_ls, bn_cd, bn_cat, bn_wc, bn_exit, bn_kill, bn_ps, bn_start_server, bn_close_server, bn_start_client, bn_send, bn_bin};    // Extra null element for 'non-builtin'
static const ssize_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(char *);

#endif

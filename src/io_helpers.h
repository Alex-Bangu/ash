#ifndef __IO_HELPERS_H__
#define __IO_HELPERS_H__

#include <sys/types.h>
#include "linked_list.h"

#define MAX_STR_LEN 31768
#define DELIMITERS " \t\n"     // Assumption: all input tokens are whitespace delimited

/* Prereq: pre_str, str are NULL terminated string
 */
void display_message(char *str);
void display_error(char *pre_str, char *str);

/* Collects and prints done messages for all background processes that have exited
 * prereq: in_ptr is not NULL
 * returns: number of messages collected
 */
int collect_done_messages(Node *in_ptr);

/* Prereq: in_ptr is a null terminated string
 * Return: 0 if in_ptr is a positive integer, 1 if in_ptr is not a positive integer
 */
int is_pos_num(char *in_ptr);

/* Expands any "..." and "...." elements in <path>
 * Prereq: <path> is null-terminated
 * Return: The new path with all expansions
 */
void expand_path(char *path, char *buffer);

/* Gets all flag info for ls and places their values into their respective variable.
 * prereq: <tokens> is an array of null-terminated strings, f_substring is of size MAX_STR_LEN
 * return: 0 if successful, -1 if an error is encountered
 */
int ls_get_flags(char ** tokens, int *d, int *rec, char *f_substring, char *path);

/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr);

/* Prereq: in_ptr points to a string of size < MAX_STR_LEN
 * Return: index of equal sign if found, -1 if no equal sign found
 */
int find_equals(char *in_ptr);

/* find_bg checks if a & sign is present at any location in the given tokens array
 * prereq: tokens is not NULL
 * return: 0 if no & is present, 1 if a & is located at the end of the last token, -1 if there are several &'s or it is not located at the front.
 */
int find_bg(char **tokens);

/* find_pipe finds a pipe sign in the given tokens array
 * prereq: tokens is not NULL
 * return: Array of token:index pairs where each pipe was found, NULL if no pipe was found
 */
int *find_pipe(char **tokens);

// Don't use this, this has no other uses besides what i'm doing with this.
int build_token_arr(char **token_arr, char ***new_token_arr, int token1, int index1);

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
int tokenize_input(char *in_ptr, char **tokens);

int freeArr(char **arr);

void expand(char *in_ptr, char *out_ptr, int *chars_written);

/* Recursive ls helper function
 * prereq: don't use this
 * return: idk, 0 if successful, -1 if it fails lol
 */
int ls_recursive(char *filepath, int depth, char *f_substring, int out_fd);

/* tokens_to_string takes in an array of NULL-terminated strings and returns a pointer
 * to a dynamically allocated string of all the tokens, each with a space separating 
 * them
 * prereq: All strings in tokens are NULL-terminated
 * return: Dynamically allocated string of all tokens, each with a space separating
 * them
 */
char *tokens_to_string(char **tokens);

#endif

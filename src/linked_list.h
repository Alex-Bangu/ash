#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

// ===== Defining Node struct =====

typedef struct node {
	char *name;
	char *value;
	struct node *next;
	long pid;
} Node;

extern Node *front;
extern Node *command_nums;

// ===== Node functions =====

/* makeNode creates a new linkedlist node on heap and assigns name and value members accordingly
 * prereq: name anid value must both be  null-terminated strings
 * return: returns a pointer to node. If memory allocation fails, a NULL pointer is returned, and and error is output to stderr
 */
Node *makeNode(char *name, char *value);

/* freeLinkedList will free all allocated memory in the linked list
 * prereq: front is a pointer to Node, ALL VARIABLE NAMES AND VALUES HELD IN THE LINKED LIST MUST BE DYNAMICALLY ALLOCATED.
 * return: void
 */
void freeLinkedList(Node *front);

/* replaceNodeVal attempts to replace the value of a pre-existing node
 * prereq: name and val are malloc'd strings and are not NULL, front is not NULL
 * return: returns 0 if successful, 1 if name does not exist in the provided linked list
 */
int replaceNodeVal(Node *front, char *name, char *new_val);

/* findValue attempts to return the value assigned to the given name
 * prereq: name is a null terminated string, front is not NULL
 * return: returns the string val assigned to the given name, if the name is not found, returns the empty string, if front is NULL, returns NULL
 */
char *findValue(char *nameToFind, Node *front);

#endif

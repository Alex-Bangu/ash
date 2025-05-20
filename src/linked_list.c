#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "linked_list.h"

// ===== Node functions =====

/* makeNode creates a new linkedlist node on heap and assigns name and value members accordingly
 * prereq: name and value must both be null-terminated strings
 * return: returns a pointer to node. If memory allocation fails, a NULL pointer is returned, and and error is output to stderr
 */
Node *makeNode(char *name, char *value) {
	Node *retnode = malloc(sizeof(Node));
	if(retnode == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for new node, aborting.\n");
		return NULL;
	}
	retnode->name = name;
	retnode->value = value;
	retnode->next = NULL;
	retnode->pid = -1;
	return retnode;
}

/* freeLinkedList will free all allocated memory in the linked list
 * prereq: front is a pointer to Node, ALL VARIABLE NAMES AND VALUES HELD IN THE LINKED LIST MUST BE DYNAMICALLY ALLOCATED.
 * return: void
 */
void freeLinkedList(Node *front) {
	Node *nextNode;
	while(front->next != NULL) {
		free(front->value);
		free(front->name);
		nextNode = front->next;
		free(front);
		front = nextNode;
	}

	free(front->value);
	free(front->name);
	free(front);
}
/* replaceNodeVal attempts to replace the value of a pre-existing node
 * prereq: name and val are malloc'd strings and are not NULL, front is not NULL
 * return: returns 0 if successful, 1 if name does not exist in the provided linked list, -1 if front is NULL.
 */
int replaceNodeVal(Node *front, char *name, char *new_val) {
	if(front == NULL) {
		fprintf(stderr, "ERROR: front Node is NULL, aborting replaceNodeVal.\n");
		return -1;
	}
	Node *curr;
	curr = front;
	int retval = 1;
	while(curr->next != NULL) {
		if(strcmp(name, curr->name) == 0) {
			free(curr->value);
			free(curr->name);          // I'm freeing and re-assigning the existing variable name because it causes a memory leak if i don't
			curr->value = new_val;
			curr->name = name;
			retval--;
		}
		curr = curr->next;
	}
	return retval;
}

/* findValue attempts to return the value assigned to nameToFind
 * prereq: nameToFind is a null terminated string, front is not NULL
 * return: returns the string val assigned to the given name, if the name is not found, returns the empty string, if front is NULL, returns NULL
 */
char *findValue(char *nameToFind, Node *front) {
	if(front == NULL) {
		fprintf(stderr, "ERROR: front node is NULL, aborting findValue.\n");
		return NULL;
	}
	while(front->next != NULL) {
		if(strcmp(front->name, nameToFind) == 0) {
			return front->value;
		} else {
			front = front->next;
		}
	}
	return front->value;
}

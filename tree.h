#ifndef TREE_H
#define TREE_H

typedef struct node {
	char  name[64];       // node's name string
	char  type;
	struct node *child, *sibling, *parent;
	// other fields if needed
} NODE;

// Function declarations for tree operations
NODE *create_node(char *name, char type);
void destroy_node(NODE *node);
void add_child(NODE *parent, NODE *child);
void remove_child(NODE *parent, NODE *child);

#endif // TREE_H
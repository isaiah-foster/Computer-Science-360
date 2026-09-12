#ifndef TREE_H
#define TREE_H

#define MAX_PATH 1024   // longest pathname the simulator handles

typedef struct node {
	char  name[64];       // node's name string
	char  type;
	struct node *child, *sibling, *parent;
	// other fields if needed
} NODE;

// Function declarations for tree operations
NODE* create_node(char *name, char type);
NODE* find_child(NODE *parent, char *name);
NODE* find_node(char *name, NODE *cwd, NODE *root);
int split_path(char *pathname, char *dirname, char *basename);
void insert_node(NODE *parent, NODE *new_node);
void remove_node(NODE *node);
void free_children(NODE *node);

#endif // TREE_H
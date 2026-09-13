#include "tree.h"
#include <stdlib.h>
#include <string.h>

// Allocate and initialize a free-standing node of the given name and type.
// Returns NULL if the allocation fails.
NODE* create_node(char *name, char type)
{
	NODE *node = (NODE*)malloc(sizeof(NODE));
	if (node == NULL)
		return NULL;

	strncpy(node->name, name, sizeof(node->name) - 1);
	node->name[sizeof(node->name) - 1] = '\0';
	node->type = type;
	node->child = NULL;
	node->sibling = NULL;
	node->parent = NULL;
	return node;
}

// Split a pathname into its dirname and basename components, e.g.
// "/a/b/c" -> dirname "/a/b", basename "c", and "c" -> dirname "" (the CWD),
// basename "c". Both buffers must hold at least MAX_PATH bytes.
// Returns 0 on success, or -1 if the pathname has no basename (i.e. "/").
int split_path(char *pathname, char *dirname, char *basename)
{
	if (pathname == NULL)
		return -1;

	// Copy the pathname so the caller's string is left untouched
	char path[MAX_PATH];
	strncpy(path, pathname, MAX_PATH - 1);
	path[MAX_PATH - 1] = '\0';

	// A trailing slash names the same node: "/a/b/" is "/a/b"
	size_t len = strlen(path);
	while (len > 1 && path[len - 1] == '/')
		path[--len] = '\0';

	char *last = strrchr(path, '/');
	if (last == NULL)        // "c": no directory part, so relative to the CWD
	{
		if (path[0] == '\0')
			return -1;
		strcpy(dirname, "");
		strcpy(basename, path);
	}
	else if (last == path)   // "/c": the directory part is the root
	{
		if (last[1] == '\0')
			return -1;       // the pathname is just "/"
		strcpy(dirname, "/");
		strcpy(basename, last + 1);
	}
	else                     // "/a/b/c" or "a/b/c"
	{
		*last = '\0';
		strcpy(dirname, path);
		strcpy(basename, last + 1);
	}
	return 0;
}

// Search a directory's child list for an entry with the given name.
// Returns the matching node, or NULL if there is none.
NODE* find_child(NODE *parent, char *name)
{
	if (parent == NULL || parent->type != 'D')
		return NULL;

	NODE *child = parent->child;
	while (child != NULL)
	{
		if (strcmp(child->name, name) == 0)
			return child;
		child = child->sibling;
	}
	return NULL;
}

// Resolve a pathname to its node in the tree.
// Absolute pathnames (leading '/') are resolved from root, relative ones
// from cwd. An empty or missing pathname resolves to cwd itself.
// Returns NULL if any component of the pathname does not exist.
NODE* find_node(char *name, NODE *cwd, NODE *root)
{
	// No pathname given: the target is the CWD
	if (name == NULL || strcmp(name, "") == 0)
		return cwd;

	NODE *current = (name[0] == '/') ? root : cwd;

	// Copy the pathname so strtok does not modify the caller's string
	char path[MAX_PATH];
	strncpy(path, name, MAX_PATH - 1);
	path[MAX_PATH - 1] = '\0';

	// Walk the tree one pathname component at a time
	char *component = strtok(path, "/");
	while (component != NULL)
	{
		if (strcmp(component, ".") == 0)
		{
			// Stay on the current node
		}
		else if (strcmp(component, "..") == 0)
		{
			// Move up one level; the root is its own parent
			current = current->parent;
		}
		else
		{
			current = find_child(current, component);
			if (current == NULL)
				return NULL;
		}
		component = strtok(NULL, "/");
	}
	return current;
}

// Add new_node as the youngest child of parent, setting the new node's
// parent and sibling links.
void insert_node(NODE *parent, NODE *new_node)
{
	if (parent == NULL || new_node == NULL)
		return;

	new_node->parent = parent;
	new_node->sibling = NULL;

	if (parent->child == NULL)
	{
		parent->child = new_node;
		return;
	}

	// Walk to the end of the child list and append there
	NODE *last = parent->child;
	while (last->sibling != NULL)
		last = last->sibling;
	last->sibling = new_node;
}

// Unlink a node from its parent's child list and free it.
// The root (its own parent) is never removed.
void remove_node(NODE *node)
{
	if (node == NULL || node->parent == node)
		return;

	NODE *parent = node->parent;
	if (parent->child == node)
	{
		parent->child = node->sibling;
	}
	else
	{
		// Find the sibling that points at this node
		NODE *prev = parent->child;
		while (prev != NULL && prev->sibling != node)
			prev = prev->sibling;
		if (prev == NULL)
			return; // node is not in its parent's list
		prev->sibling = node->sibling;
	}

	free(node);
}

// Recursively free every descendant of a node, leaving the node childless.
void free_children(NODE *node)
{
	if (node == NULL)
		return;

	NODE *child = node->child;
	while (child != NULL)
	{
		NODE *next = child->sibling;
		free_children(child);
		free(child);
		child = next;
	}
	node->child = NULL;
}

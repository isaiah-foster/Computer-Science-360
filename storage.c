#include "storage.h"
#include <stdio.h>
#include <string.h>

// Write one "TYPE PATH" line for a node, then for each of its descendants.
static void save_node(FILE *fp, NODE *node, char *path)
{
	fprintf(fp, "%c %s\n", node->type, path);

	int at_root = (strcmp(path, "/") == 0);
	char child_path[MAX_PATH];

	NODE *child = node->child;
	while (child != NULL)
	{
		// The root already ends in '/', so don't add a second one
		snprintf(child_path, sizeof(child_path), "%s%s%s",
			path, at_root ? "" : "/", child->name);
		save_node(fp, child, child_path);
		child = child->sibling;
	}
}

// Write the whole filesystem tree to filename.
int save_to_file(char *filename, NODE *root)
{
	FILE *fp = fopen(filename, "w+");
	if (fp == NULL)
	{
		printf("Cannot open file %s!\n", filename);
		return -1;
	}

	save_node(fp, root, "/");
	fclose(fp);
	return 0;
}

// Rebuild the filesystem tree under root from a file written by save_to_file.
int load_from_file(char *filename, NODE *root)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
	{
		printf("Cannot open file %s!\n", filename);
		return -1;   // leave the current tree alone
	}

	// drop the old tree and start ove
	free_children(root);

	char line[MAX_PATH + 8];
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		// Each line reads "TYPE PATH", e.g. "D /a/b"
		char *path = line;
		while (*path == ' ' || *path == '\t')
			path++;

		char type = *path++;
		if (type != 'D' && type != 'F')
			continue; // skip blank or malformed lines

		while (*path == ' ' || *path == '\t')
			path++;

		// Drop the newline and any other trailing whitespace
		size_t len = strlen(path);
		while (len > 0 && (path[len - 1] == '\n' || path[len - 1] == '\r' ||
			path[len - 1] == ' ' || path[len - 1] == '\t'))
			path[--len] = '\0';

		char dirname[MAX_PATH], basename[MAX_PATH];
		if (split_path(path, dirname, basename) != 0)
			continue;// the root line: it already exists

		// Both searches start at the root because saved paths are absolute
		NODE *parent = find_node(dirname, root, root);
		if (parent == NULL || parent->type != 'D')
			continue; // parent line missing, so skip this entry
		if (find_child(parent, basename) != NULL)
			continue;  // duplicate line

		NODE *node = create_node(basename, type);
		if (node == NULL)
		{
			printf("Memory allocation failed for new node.\n");
			break;
		}
		insert_node(parent, node);
	}

	fclose(fp);
	return 0;
}

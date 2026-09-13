#include "command.h"
#include "storage.h"
#include <stdlib.h>
#include <stdio.h>

// Where the quit command saves the filesystem tree
#define QUIT_SAVE_FILE "fssim_foster.txt"

// List of all available commands
char *cmd[] = { 
    "mkdir", "rmdir",
    "ls", "cd",
    "pwd", "creat",
    "rm", "reload",
    "save", "quit", 0 
};

char** parse_command(char *user_command)
{
	char **tokens = (char**)malloc(3 * sizeof(char*));
	if (!tokens)
	{
		printf("Memory allocation failed for tokens.\n");
		return NULL;
	} 

	char *input_copy = (char*)malloc(strlen(user_command) + 1);
	if (!input_copy)
	{
		printf("Memory allocation failed for input copy.\n");
		free(tokens);
		return NULL;
	}
	strcpy(input_copy, user_command);

	int token_count = 0;
	char *token = strtok(input_copy, " \t\n");

	while(token != NULL && token_count < 3)
	{
		if (token_count > 1) {
			printf("unexpected argument: %s\n", token);
			for (int i = 0; i < token_count; i++)
				free(tokens[i]);
			free(tokens);
			free(input_copy);
			return NULL;
		}

		tokens[token_count] = (char*)malloc(strlen(token) + 1);
		if (tokens[token_count])
			strcpy(tokens[token_count], token);
		token_count++;
		token = strtok(NULL, " \t\n");
	}

	tokens[token_count] = NULL;
	free(input_copy);

	if (token_count == 0) {
		printf("No command entered.\n");
		free(tokens);
		return NULL;
	}
	return tokens;
}

int execute_command(char **tokens, NODE **cwd, NODE *root)
{
	if (tokens == NULL || tokens[0] == NULL) {
		printf("No command entered.\n");
		return -1;
	}

	for (int i = 0; cmd[i] != 0; i++) {
		if (strcmp(tokens[0], cmd[i]) == 0) {
			switch(i) {
				case 0: return mkdir_command(tokens[1], *cwd, root);
				case 1: return rmdir_command(tokens[1], *cwd, root);
				case 2: return ls_command(tokens[1], *cwd, root);
				case 3: return cd_command(tokens[1], cwd, root);
				case 4: return pwd_command(*cwd);
				case 5: return creat_command(tokens[1], *cwd, root);
				case 6: return rm_command(tokens[1], *cwd, root);
				case 7: return reload_command(tokens[1], cwd, root);
				case 8: return save_command(tokens[1], root);
				case 9: return quit_command(root);
			}
		}
	}

	printf("Command not found!\n");
	return -1;
}

int mkdir_command(char *name, NODE *cwd, NODE *root)
{
	if (name == NULL)
	{
		printf("Usage: mkdir pathname\n");
		return -1;
	}

	// Split the pathname, e.g. /a/b/c into dirname /a/b and basename c
	char dirname[MAX_PATH], basename[MAX_PATH];
	if (split_path(name, dirname, basename) != 0)
	{
		// The only pathname with no basename is the root, which always exists
		printf("DIR %s already exists!\n", name);
		return -1;
	}

	// The parent directory has to exist and be a DIR
	NODE* parent = find_node(dirname, cwd, root);
	if (parent == NULL)
	{
		printf("No such file or directory: %s\n", dirname);
		return -1;
	}
	if (parent->type != 'D')
	{
		printf("Not a directory: %s\n", dirname);
		return -1;
	}

	// Check if the directory already exists
	if (find_child(parent, basename) != NULL)
	{
		printf("DIR %s already exists!\n", name);
		return -1;
	}

	NODE* new_node = create_node(basename, 'D'); // Set type to directory
	if (!new_node)
	{
		printf("Memory allocation failed for new node.\n");
		return -1;
	}

	// Insert new node into the tree
	insert_node(parent, new_node);
	return 0;
}

int rmdir_command(char *name, NODE *cwd, NODE *root)
{
	if (name == NULL)
	{
		printf("Usage: rmdir pathname\n");
		return -1;
	}

	NODE* node = find_node(name, cwd, root);
	if (node == NULL || node->type != 'D') // Check if the node exists and is a directory
	{
		printf("DIR %s does not exist!\n", name);
		return -1;
	}
	else if (node->parent == NULL) // The root is never removed
	{
		printf("Cannot remove DIR %s!\n", name);
		return -1;
	}
	else
	{
		if (node->child == NULL) // Check if it's a directory and empty
		{
			remove_node(node);
			return 0;
		}
		else
		{
			printf("Cannot remove DIR %s (not empty)!\n", name);
			return -1;
		}
	}
}

int ls_command(char *name, NODE *cwd, NODE *root)
{
	NODE* node = find_node(name, cwd, root);
	if (node == NULL)
	{
		printf("No such file or directory: %s\n", name);
		return -1;
	}
	else
	{
		if (node->type == 'D') // If it's a directory list contents
		{
			NODE* child = node->child;
			while (child != NULL)
			{
				printf("%c %s\n", child->type, child->name);
				child = child->sibling;
			}
		}
		else // If it's a file print its name
		{
			printf("%c %s\n", node->type, node->name);
		}
	}
	return 0;
}

int cd_command(char *name, NODE **cwd, NODE *root)
{
	// If name is NULL, empty, or "/" change to root
	if (name == NULL || strcmp(name, "") == 0 || strcmp(name, "/") == 0)
	{
		*cwd = root; 
		return 0;
	}

	// Else look for actual path
	NODE* node = find_node(name, *cwd, root);
	if (node == NULL)
	{
		printf("No such file or directory: %s\n", name);
		return -1;
	}
	else
	{
		if (node->type == 'D')
		{
			*cwd = node;
			return 0;
		}
		else
		{
			printf("Not a directory: %s\n", name);
			return -1;
		}
	}
}

// Recursively print the absolute pathname of a node, from the root down.
// The root itself contributes nothing, so its children print as "/name".
static void print_path(NODE *node)
{
	if (node == NULL || node->parent == NULL)
		return;
	print_path(node->parent);
	printf("/%s", node->name);
}

int pwd_command(NODE *cwd)
{
	if (cwd->parent == NULL) // The CWD is the root
		printf("/\n");
	else
	{
		print_path(cwd);
		printf("\n");
	}
	return 0;
}

int creat_command(char *name, NODE *cwd, NODE *root)
{
	if (name == NULL)
	{
		printf("Usage: creat pathname\n");
		return -1;
	}

	// Split the pathname, e.g. /a/b/c into dirname /a/b and basename c
	char dirname[MAX_PATH], basename[MAX_PATH];
	if (split_path(name, dirname, basename) != 0)
	{
		// The only pathname with no basename is the root, which always exists
		printf("%s already exists!\n", name);
		return -1;
	}

	// The parent directory has to exist and be a DIR
	NODE* parent = find_node(dirname, cwd, root);
	if (parent == NULL)
	{
		printf("No such file or directory: %s\n", dirname);
		return -1;
	}
	if (parent->type != 'D')
	{
		printf("Not a directory: %s\n", dirname);
		return -1;
	}

	// Check if the file already exists
	if (find_child(parent, basename) != NULL)
	{
		printf("%s already exists!\n", name);
		return -1;
	}

	NODE* new_node = create_node(basename, 'F'); // Set type to file
	if (!new_node)
	{
		printf("Memory allocation failed for new node.\n");
		return -1;
	}

	// Insert new node into the tree
	insert_node(parent, new_node);
	return 0;
}

int rm_command(char *name, NODE *cwd, NODE *root)
{
	if (name == NULL)
	{
		printf("Usage: rm pathname\n");
		return -1;
	}

	NODE* node = find_node(name, cwd, root);
	if (node == NULL) // Check if the node exists
	{
		printf("File %s does not exist!\n", name);
		return -1;
	}
	else if (node->type != 'F') // Check that it is a file and not a directory
	{
		printf("Cannot remove %s (not a FILE)!\n", name);
		return -1;
	}
	else
	{
		remove_node(node);
		return 0;
	}
}

int reload_command(char *filename, NODE **cwd, NODE *root)
{
	if (filename == NULL)
	{
		printf("Usage: reload filename\n");
		return -1;
	}

	// A successful load frees the old tree, so move the CWD back to the root
	// before it can be left pointing at a node that no longer exists
	if (load_from_file(filename, root) != 0)
		return -1;

	*cwd = root;
	return 0;
}

int save_command(char *filename, NODE *root)
{
	if (filename == NULL)
	{
		printf("Usage: save filename\n");
		return -1;
	}

	return save_to_file(filename, root);
}

int quit_command(NODE *root)
{
	// Save the tree under the default name, then terminate the program
	save_to_file(QUIT_SAVE_FILE, root);
	exit(0);
	return 0;
}
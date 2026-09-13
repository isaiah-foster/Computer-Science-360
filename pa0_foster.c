#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "storage.h"
#include "tree.h"

int main() {
	// Initialize root
	NODE *root;
	root = (NODE *)malloc(sizeof(NODE));
	strcpy(root->name, "/");
	root->parent = root;
	root->child = NULL;
	root->parent = NULL;
	root->type = 'D';

	// Initialize current working directory
	NODE *cwd;
	cwd = (NODE *)malloc(sizeof(NODE));
	cwd = root;

	/* USED AI FOR ANSI ESCAPE SEQUENCES 
	 * Prompt: wrap this line of code in ANSI escape sequences.
	 * I messed with the numbers to match my terminal's colors. Same applies to line 36
	 */
	printf("\n\033[1;36mWelcome to the Unix File System!\n\n\033[0m");
	
	// Initialize user command buffers
	char user_cmd[1024];
	char **parsed_cmd;

	while(1)
	{
		// Prompt user for command
		printf("\033[1;34m%s%s\033[0m", cwd->name, " > ");
		fgets(user_cmd, sizeof(user_cmd), stdin);
		size_t len = strlen(user_cmd);
		if (len > 0 && user_cmd[len - 1] == '\n')
			user_cmd[len - 1] = '\0';

		// Parse and execute command
		parsed_cmd = parse_command(user_cmd);
		if (parsed_cmd == NULL) continue;
		execute_command(parsed_cmd, &cwd, root);

		// Free tokens
		for (int i = 0; parsed_cmd[i] != NULL; i++)
			free(parsed_cmd[i]);
		free(parsed_cmd);
	}

	return 0;
}

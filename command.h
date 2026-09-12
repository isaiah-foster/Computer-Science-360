#ifndef COMMANDS_H
#define COMMANDS_H

#include <string.h>
#include "tree.h"

// Parse full user input into tokens
char** parse_command(char *user_command);

// Execute the command based on parsed tokens
int execute_command(char **tokens, NODE **cwd, NODE *root);

// Command implementations
int mkdir_command(char *name);
int rmdir_command(char *name);
int ls_command(char *name, NODE *cwd, NODE *root);
int cd_command(char *name, NODE **cwd, NODE *root);
int pwd_command(NODE *cwd);
int creat_command(char *name);
int rm_command(char *name, NODE *cwd, NODE *root);
int reload_command(char *filename);
int save_command(char *filename);

#endif // COMMANDS_H
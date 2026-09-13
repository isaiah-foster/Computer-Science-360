#ifndef COMMAND_H
#define COMMAND_H

#include <string.h>
#include "tree.h"

// Parse full user input into tokens
char** parse_command(char *user_command);

// Execute the command based on parsed tokens
int execute_command(char **tokens, NODE **cwd, NODE *root);

// Command implementations
int mkdir_command(char *name, NODE *cwd, NODE *root);
int rmdir_command(char *name, NODE *cwd, NODE *root);
int ls_command(char *name, NODE *cwd, NODE *root);
int cd_command(char *name, NODE **cwd, NODE *root);
int pwd_command(NODE *cwd);
int creat_command(char *name, NODE *cwd, NODE *root);
int rm_command(char *name, NODE *cwd, NODE *root);
int reload_command(char *filename, NODE **cwd, NODE *root);
int save_command(char *filename, NODE *root);
int quit_command(NODE *root);

#endif // COMMAND_H
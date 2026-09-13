#ifndef STORAGE_H
#define STORAGE_H

#include "tree.h"

// Storage-related functions and declarations
int load_from_file(char *filename, NODE *root);
int save_to_file(char *filename, NODE *root);

#endif // STORAGE_H

#ifndef __UTILS__
#define __UTILS__

#include "darknet.h"
#include "list.h"

void malloc_error();
void file_error(char *s);
char *fgetl(FILE *fp);
void strip(char *s);
void error(const char *s);
int int_index(int *a, int val, int n);
float mag_array(float *a, int n);
float rand_normal();
int *read_map(char *filename);
float sum_array(float *a, int n);
list *get_paths(char *filename);

#endif

#ifndef __OPTIONS__
#define __OPTIONS__

#include "darknet.h"
#include "list.h"
#include "utils.h"

float option_find_float(list *l, char *key, float def);
void option_insert(list *l, char *key, char *val);
int read_option(char *s, list *options);
int option_find_int(list *l, char *key, int def);
char *option_find(list *l, char *key);
float option_find_float_quiet(list *l, char *key, float def);
int option_find_int_quiet(list *l, char *key, int def);
void option_unused(list *l);

#endif

#ifndef __LIST__
#define __LIST__

#include "darknet.h"

list *make_list();
void list_insert(list *l, void *val);
void free_node(node *n);
void free_list(list *l);
void **list_to_array(list *l);

#endif

#ifndef __PARSER__
#define __PARSER__

#include "darknet.h"
#include "options.h"
#include "box.h"
#include "network.h"
#include "utils.h"
#include "image.h"

int is_network(section *s);
void transpose_matrix(float *a, int rows, int cols);
convolutional_layer parse_convolutional(list *options, size_params params);
int *parse_yolo_mask(char *a, int *num);
layer parse_yolo(list *options, size_params params);
maxpool_layer parse_maxpool(list *options, size_params params);
route_layer parse_route(list *options, size_params params, network *net);
layer parse_upsample(list *options, size_params params, network *net);
void load_convolutional_weights(layer l, FILE *fp);
learning_rate_policy get_policy(char *s);
void parse_net_options(list *options, network *net);
void free_section(section *s);
void free_layer(layer l);
LAYER_TYPE string_to_layer_type(char * type);
network *parse_network_cfg(char *filename);
void load_weights(network *net, char *filename);
void load_weights_upto(network *net, char *filename, int start, int cutoff);
list *read_cfg(char *filename);

#endif

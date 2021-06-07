#ifndef __NETWORK__
#define __NETWORK__

#include "darknet.h"
#include "convolution.h"
#include "maxpool.h"
#include "route.h"
#include "upsample.h"
#include "yolo.h"
#include "utils.h"
#include "parser.h"
#include "blas.h"

layer get_network_output_layer(network *net);
network *make_network(int n);
void forward_network(network *netp);
void calc_network_cost(network *netp);
void fill_network_boxes(network *net, int w, int h, float thresh, float hier, int *map, int relative, detection *dets);
detection *make_network_boxes(network *net, float thresh, int *num);
int num_detections(network *net, float thresh);

#endif

#ifndef __ROUTE__
#define __ROUTE__

#include "darknet.h"
#include "blas.h"

route_layer make_route_layer(int batch, int n, int *input_layers, int *input_sizes);
void backward_route_layer(const route_layer l, network net);
void forward_route_layer(const route_layer l, network net);

#endif

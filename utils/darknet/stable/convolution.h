#ifndef __CONVOLUTION__
#define __CONVOLUTION__

#include "darknet.h"
#include "image.h"
#include "activate.h"
#include "network.h"
#include "utils.h"
#include "batchnorm.h"

convolutional_layer make_convolutional_layer(int batch, int h, int w, int c, int n, int groups, int size, int stride, int padding, ACTIVATION activation, int batch_normalize, int binary, int xnor, int adam);
void resize_convolutional_layer(convolutional_layer *layer, int w, int h);
void forward_convolutional_layer(const convolutional_layer layer, network net);
void update_convolutional_layer(convolutional_layer layer, update_args a);

#endif

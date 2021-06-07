#ifndef __BATCHNORM__
#define __BATCHNORM__

#include "darknet.h"
#include "blas.h"
#include "utils.h"

void forward_batchnorm_layer(layer l, network net);
void backward_batchnorm_layer(layer l, network net);
void add_bias(float *output, float *biases, int batch, int n, int size);
void scale_bias(float *output, float *scales, int batch, int n, int size);
void backward_bias(float *bias_updates, float *delta, int batch, int n, int size);

#endif

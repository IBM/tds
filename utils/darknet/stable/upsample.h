#ifndef __UPSAMPLE__
#define __UPSAMPLE__

#include "darknet.h"
#include "blas.h"

layer make_upsample_layer(int batch, int w, int h, int c, int stride);
void forward_upsample_layer(const layer l, network net);
void backward_upsample_layer(const layer l, network net);

#endif

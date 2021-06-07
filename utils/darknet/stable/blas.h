#ifndef __BLAS__
#define __BLAS__

#include "darknet.h"

void fill_cpu(int N, float ALPHA, float *X, int INCX);
void mean_cpu(float *x, int batch, int filters, int spatial, float *mean);
void axpy_cpu(int N, float ALPHA, float *X, int INCX, float *Y, int INCY);
void scal_cpu(int N, float ALPHA, float *X, int INCX);
void copy_cpu(int N, float *X, int INCX, float *Y, int INCY);
void upsample_cpu(float *in, int w, int h, int c, int batch, int stride, int forward, float scale, float *out);
void variance_cpu(float *x, float *mean, int batch, int filters, int spatial, float *variance);
void normalize_cpu(float *x, float *mean, float *variance, int batch, int filters, int spatial);

#endif

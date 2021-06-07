#ifndef __YOLO__
#define __YOLO__

#include "darknet.h"
#include "blas.h"
#include "utils.h"
#include "box.h"
#include "activate.h"

int yolo_num_detections(layer l, float thresh);
void avg_flipped_yolo(layer l);
int get_yolo_detections(layer l, int w, int h, int netw, int neth, float thresh, int *map, int relative, detection *dets);
box get_yolo_box(float *x, float *biases, int n, int index, int i, int j, int lw, int lh, int w, int h, int stride);
void correct_yolo_boxes(detection *dets, int n, int w, int h, int netw, int neth, int relative);
layer make_yolo_layer(int batch, int w, int h, int n, int total, int *mask, int classes);

#endif

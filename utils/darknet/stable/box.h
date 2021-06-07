#ifndef __BOX__
#define __BOX__

#include "darknet.h"

int nms_comparator(const void *pa, const void *pb);
float overlap(float x1, float w1, float x2, float w2);
float box_intersection(box a, box b);
float box_union(box a, box b);
float box_iou(box a, box b);
box float_to_box(float *f, int stride);

#endif

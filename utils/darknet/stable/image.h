#ifndef __IMAGE__
#define __IMAGE__

#include "darknet.h"
#include "utils.h"
#include "blas.h"

image threshold_image(image im, float thresh);
image float_to_image(int w, int h, int c, float *data);
void draw_label(image a, int r, int c, image label, const float *rgb);
void composite_image(image source, image dest, int dx, int dy);
image copy_image(image p);
image border_image(image a, int border);
image tile_images(image a, image b, int dx);
image get_label(image **characters, char *string, int size);
void draw_box(image a, int x1, int y1, int x2, int y2, float r, float g, float b);
void draw_box_width(image a, int x1, int y1, int x2, int y2, int w, float r, float g, float b);
float get_color(int c, int x, int max);
void embed_image(image source, image dest, int dx, int dy);
void fill_image(image m, float s);
void draw_box_width(image a, int x1, int y1, int x2, int y2, int w, float r, float g, float b);
image make_empty_image(int w, int h, int c);
image resize_image(image im, int w, int h);
image load_image_stb(char *filename, int channels);
image load_image(char *filename, int w, int h, int c);
image load_image_color(char *filename, int w, int h);
void save_image_options(image im, const char *name, IMTYPE f, int quality);

#endif

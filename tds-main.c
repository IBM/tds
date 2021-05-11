/*
 * Copyright 2021 IBM
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "darknet.h"
#include "utils/microjson-1.6/mjson.h"

#define FFPROBE_CMD "ffprobe -v error -show_entries stream=width,height -of default=noprint_wrappers=1:nokey=1 %s"
#define FFMPEG_CMD  "ffmpeg -hide_banner -loglevel error -rtsp_transport tcp -i %s -filter:v fps=0.25 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -"

static int coco_ids[] = {1,2,3,4,5,6,7,8,9,10,11,13,14,15,16,17,18,19,20,21,22,23,24,25,27,28,31,32,33,34,35,36,37,38,39,40,41,42,43,44,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,67,70,72,73,74,75,76,77,78,79,80,81,82,84,85,86,87,88,89,90};

bool exit_loop = false;

typedef struct {
  char darknet_home[512];
  char darknet_datacfg[512];
  char darknet_cfgfile[512];
  char darknet_weightfile[512];
  char input_image[512];
  char input_stream_1[512];
  char input_stream_2[512];
  char input_stream_3[512];
  char input_stream_4[512];
  char input_stream_5[512];
  char input_stream_6[512];
  bool use_input_image;
  bool use_input_stream;
} conf_params_t;

typedef struct {
  int width;
  int height;
  int c;
} dim_t;

// The application supports up to 6 input streams
typedef struct {
  FILE *pipein_1;
  FILE *pipein_2;
  FILE *pipein_3;
  FILE *pipein_4;
  FILE *pipein_5;
  FILE *pipein_6;
} input_t;


int parse_config_file(char *filename, conf_params_t *conf_params)
{
  printf("Config file:   %s\n", filename);

  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
      printf("ERROR: cannot open %s\n", filename);
      return -1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
  char *string = malloc(fsize + 1);
  fread(string, 1, fsize, f);
  fclose(f);
  string[fsize] = 0;

  /* Mapping of JSON attributes to C my_object's struct members */
  const struct json_attr_t json_attrs[] = {
       {"darknet_home", t_string, .addr.string = conf_params->darknet_home, .len = sizeof(conf_params->darknet_home)},
       {"darknet_datacfg", t_string, .addr.string = conf_params->darknet_datacfg, .len = sizeof(conf_params->darknet_datacfg)},
       {"darknet_cfgfile", t_string, .addr.string = conf_params->darknet_cfgfile, .len = sizeof(conf_params->darknet_cfgfile)},
       {"darknet_weightfile", t_string, .addr.string = conf_params->darknet_weightfile, .len = sizeof(conf_params->darknet_weightfile)},
       {"input_stream_1", t_string, .addr.string = conf_params->input_stream_1, .len = sizeof(conf_params->input_stream_1)},
       {"input_stream_2", t_string, .addr.string = conf_params->input_stream_2, .len = sizeof(conf_params->input_stream_2)},
       {"input_stream_3", t_string, .addr.string = conf_params->input_stream_3, .len = sizeof(conf_params->input_stream_3)},
       {"input_stream_4", t_string, .addr.string = conf_params->input_stream_4, .len = sizeof(conf_params->input_stream_4)},
       {"input_stream_5", t_string, .addr.string = conf_params->input_stream_5, .len = sizeof(conf_params->input_stream_5)},
       {"input_stream_6", t_string, .addr.string = conf_params->input_stream_6, .len = sizeof(conf_params->input_stream_6)},
       {"input_image", t_string, .addr.string = conf_params->input_image, .len = sizeof(conf_params->input_image)},
       {NULL},
     };

  /* Parse the JSON string from buffer */
  int status = json_read_object(string, json_attrs, NULL);
  if (status != 0) {
    puts(json_error_string(status));
    return -1;
  }

  conf_params->use_input_image  = false;
  conf_params->use_input_stream = false;

  if (conf_params->input_image[0] != '\0')
    // Use image file
    conf_params->use_input_image = true;
  else
    if ((conf_params->input_stream_1[0] != '\0') || (conf_params->input_stream_2[0] != '\0') || (conf_params->input_stream_3[0] != '\0') ||
	(conf_params->input_stream_4[0] != '\0') || (conf_params->input_stream_5[0] != '\0') || (conf_params->input_stream_6[0] != '\0') ) {
      // Use RTSP video stream(s)
      conf_params->use_input_stream = true;
      conf_params->use_input_image  = false;
    }

  if (!conf_params->use_input_image && !conf_params->use_input_stream) {
    printf("ERROR: both input_image and input_stream_* cannot be empty in the configuration file\n");
    return -1;
  }

  return 0;
}


int get_input_dimensions(conf_params_t conf_params, dim_t *dimensions)
{
  char ffprobe_cmd[1024];
  if (conf_params.use_input_image)
    // Use image file
    snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_image);
  else
    // Use RTSP video stream
    if (conf_params.input_stream_1[0] != '\0')
      snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_stream_1);
    else if (conf_params.input_stream_2[0] != '\0')
      snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_stream_2);
    else if (conf_params.input_stream_3[0] != '\0')
      snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_stream_3);
    else if (conf_params.input_stream_4[0] != '\0')
      snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_stream_4);
    else if (conf_params.input_stream_5[0] != '\0')
      snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_stream_5);
    else if (conf_params.input_stream_6[0] != '\0')
      snprintf(ffprobe_cmd, 1024, FFPROBE_CMD, conf_params.input_stream_6);

  FILE * pipein = popen(ffprobe_cmd, "r");
  if (pipein == NULL) {
    printf("ERROR: failed to run ffprobe command\n" );
    return -1;
  }
  char line[8];
  fgets(line, sizeof(line), pipein);
  dimensions->width = atoi(line);
  fgets(line, sizeof(line), pipein);
  dimensions->height = atoi(line);
  printf("Dimensions:    %dx%d\n", dimensions->width, dimensions->height);
  dimensions->c = 3;
  pclose(pipein);

  return 0;
}


int open_input_pipes(conf_params_t conf_params, input_t *input)
{
  input->pipein_1 = NULL;
  input->pipein_2 = NULL;
  input->pipein_3 = NULL;
  input->pipein_4 = NULL;
  input->pipein_5 = NULL;
  input->pipein_6 = NULL;

  char ffmpeg_cmd[1024];
  if (conf_params.use_input_image) {
    // Use image file
    snprintf(ffmpeg_cmd, 1024, "ffmpeg -hide_banner -loglevel error -i %s -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", conf_params.input_image);
    input->pipein_1 = popen(ffmpeg_cmd, "r");
    // We just iterate once if we're reading from one single image file
    exit_loop = true;
  }
  else {
    // Use RTSP video stream 1
    if (conf_params.input_stream_1[0] != '\0') {
      snprintf(ffmpeg_cmd, 1024, FFMPEG_CMD, conf_params.input_stream_1);
      input->pipein_1 = popen(ffmpeg_cmd, "r");
    }
    // Use RTSP video stream 2
    if (conf_params.input_stream_2[0] != '\0') {
      snprintf(ffmpeg_cmd, 1024, FFMPEG_CMD, conf_params.input_stream_2);
      input->pipein_2 = popen(ffmpeg_cmd, "r");
    }
    // Use RTSP video stream 3
    if (conf_params.input_stream_3[0] != '\0') {
      snprintf(ffmpeg_cmd, 1024, FFMPEG_CMD, conf_params.input_stream_3);
      input->pipein_3 = popen(ffmpeg_cmd, "r");
    }
    // Use RTSP video stream 4
    if (conf_params.input_stream_4[0] != '\0') {
      snprintf(ffmpeg_cmd, 1024, FFMPEG_CMD, conf_params.input_stream_4);
      input->pipein_4 = popen(ffmpeg_cmd, "r");
    }
    // Use RTSP video stream 5
    if (conf_params.input_stream_5[0] != '\0') {
      snprintf(ffmpeg_cmd, 1024, FFMPEG_CMD, conf_params.input_stream_5);
      input->pipein_5 = popen(ffmpeg_cmd, "r");
    }
    // Use RTSP video stream 6
    if (conf_params.input_stream_6[0] != '\0') {
      snprintf(ffmpeg_cmd, 1024, FFMPEG_CMD, conf_params.input_stream_6);
      input->pipein_6 = popen(ffmpeg_cmd, "r");
    }
  }

  printf("pipein_1 = %p\n", (void *)input->pipein_1);
  printf("pipein_2 = %p\n", (void *)input->pipein_2);
  printf("pipein_3 = %p\n", (void *)input->pipein_3);
  printf("pipein_4 = %p\n", (void *)input->pipein_4);
  printf("pipein_5 = %p\n", (void *)input->pipein_5);
  printf("pipein_6 = %p\n", (void *)input->pipein_6);

  return 0;
}


void close_input_pipes(input_t input)
{
  // Flush and close input and output pipes
  if (input.pipein_1 != NULL) {
      fflush(input.pipein_1);
      pclose(input.pipein_1);
  }
  if (input.pipein_2 != NULL) {
      fflush(input.pipein_2);
      pclose(input.pipein_2);
  }
  if (input.pipein_3 != NULL) {
      fflush(input.pipein_3);
      pclose(input.pipein_3);
  }
  if (input.pipein_4 != NULL) {
      fflush(input.pipein_4);
      pclose(input.pipein_4);
  }
  if (input.pipein_5 != NULL) {
      fflush(input.pipein_5);
      pclose(input.pipein_5);
  }
  if (input.pipein_6 != NULL) {
      fflush(input.pipein_6);
      pclose(input.pipein_6);
  }
}


FILE *get_pipe(int i, input_t input)
{
  switch (i) {
    case 1: return input.pipein_1;
    case 2: return input.pipein_2;
    case 3: return input.pipein_3;
    case 4: return input.pipein_4;
    case 5: return input.pipein_5;
    case 6: return input.pipein_6;
  }
  assert(0);
}


void sig_handler(int signo)
{
  if (signo == SIGINT) {
    exit_loop = true;
    fflush(stdout);
    fflush(stderr);
  }
}


int main(int argc, char *argv[])
{

  if (argc < 2 || argc > 3) {
    printf("Usage: ./mas-demo <JSON config file> [log subdir]\n");
    exit(-1);
  }

  signal(SIGINT, sig_handler);
  printf("PID:           %d\n", getpid());


  /*************************************************************************************/
  /* Create sub-directory for this specific run (we chdir later in the code below)     */
  /*************************************************************************************/
  char *dir_name;
  if (argc == 3)
    // The use provided a subdir name; let's use it (we assumed it was already created)
    dir_name = argv[2];
  else
    dir_name = ".";

  // Check if directory exists
  DIR* dir = opendir(dir_name);
  if (dir)
    // Directory exists
    closedir(dir);
  else if (ENOENT == errno) {
    // Directory does not exist
    printf("ERROR: directory %s doesn't exist\n", dir_name);
    exit(-1);
  }
  printf("Run directory: %s\n", dir_name);


  /*************************************************************************************/
  /* Parse JSON configuration file                                                     */
  /*************************************************************************************/
  conf_params_t conf_params;
  if (parse_config_file(argv[1], &conf_params) != 0) {
    printf("ERROR: cannot parse JSON configuration file\n");
    exit(-1);
  }


  /*************************************************************************************/
  /* Determine input's dimensions (width and height)                                   */
  /*************************************************************************************/
  dim_t dimensions;
  if (get_input_dimensions(conf_params, &dimensions) != 0) {
    printf("ERROR: cannot determine input's dimensions (width and height)\n");
    exit(-1);
  }


  /*************************************************************************************/
  /* Initialize Darknet model                                                          */
  /*************************************************************************************/
  char datacfg[1024];
  char cfgfile[1024];
  char weightfile[1024];
  snprintf(datacfg,    1024, "%s/%s", conf_params.darknet_home, conf_params.darknet_datacfg);
  snprintf(cfgfile,    1024, "%s/%s", conf_params.darknet_home, conf_params.darknet_cfgfile);
  snprintf(weightfile, 1024, "%s/%s", conf_params.darknet_home, conf_params.darknet_weightfile);
  float thresh       = .5;
  float hier_thresh  = .5;

  printf("Darknet home:  %s\n", conf_params.darknet_home);
  printf("Data config:   %s\n", datacfg);
  printf("Model config:  %s\n", cfgfile);
  printf("Weights:       %s\n", weightfile);
  printf("\n");

  list *options = read_data_cfg(datacfg);
  metadata meta = get_metadata(datacfg);
  char *name_list = option_find_str(options, "names", "names.list");
  char **names = get_labels(name_list);

  image **alphabet = load_alphabet();
  network *net = load_network(cfgfile, weightfile, 0);
  set_batch_network(net, 1);
  srand(2222222);
  double curr_time;
  float nms=.45;

#ifdef NNPACK
  nnp_initialize();
  net->threadpool = pthreadpool_create(4);
#endif


  /*************************************************************************************/
  /* Create pipe to read from video/image source                                       */
  /*************************************************************************************/
  input_t input;
  if (open_input_pipes(conf_params, &input) != 0){
    printf("ERROR: cannot open input pipe(s)\n");
    exit(-1);
  }

  unsigned char *data = malloc(sizeof(unsigned char)*dimensions.width*dimensions.height*dimensions.c);
  unsigned long count = 0, i = 0;

  // Some statistics
  double read_time       = 0;
  double conversion_time = 0;
  double prediction_time = 0;
  double boxing_time     = 0;

  FILE *pipein;
  int pipe_id;
  char outfile[300];
  time_t timestamp;
  bool object_detected;

  chdir(dir_name);
  FILE *fp = fopen("predictions.log", "w");
  fprintf(fp, "cam_id,time,object_id,object_name,prob,avg_read_time_sec,avg_conv_time_sec,avg_pred_time_sec,bbox_time_sec\n");

  do {

    /*************************************************************************************/
    /* Capture image to process                                                          */
    /*************************************************************************************/
    pipe_id = (i % 6) + 1;
    pipein  = get_pipe(pipe_id, input);
    i++;
    if (pipein == NULL) continue;
    printf("Reading from pipe %d (%p)\n", pipe_id, (void *)pipein);
    curr_time = what_time_is_it_now();
    size_t size = fread(data, 1, dimensions.width*dimensions.height*dimensions.c, pipein);
    read_time = (what_time_is_it_now()-curr_time);
    //printf("Data read in %f seconds.\n", what_time_is_it_now()-curr_time);
    if (size != dimensions.width*dimensions.height*dimensions.c) {
      printf("Warning: %zu bytes read (expected: %d)!\n", size, dimensions.width*dimensions.height*dimensions.c);
      sleep(1);
      continue;
    }

    // Convert raw image into YOLO/Darknet image format
    curr_time = what_time_is_it_now();
    int i,j,k;
    image im = make_image(dimensions.width, dimensions.height, dimensions.c);
    for (k = 0; k < dimensions.c; ++k) {
        for (j = 0; j < dimensions.height; ++j) {
  	  for (i = 0; i < dimensions.width; ++i) {
  	      int dst_index = i + dimensions.width*j + dimensions.width*dimensions.height*k;
  	      int src_index = k + dimensions.c*i + dimensions.c*dimensions.width*j;
  	      im.data[dst_index] = (float)data[src_index]/255.;
  	  }
        }
    }
    image sized     = letterbox_image(im, net->w, net->h);
    float *X        = sized.data;
    conversion_time = (what_time_is_it_now()-curr_time);
    //printf("Image converted in %f seconds.\n", what_time_is_it_now()-curr_time);


    /*************************************************************************************/
    /* Detect objects                                                                    */
    /*************************************************************************************/
    curr_time = what_time_is_it_now();
    network_predict(net, X);
    //printf("Predicted in %f seconds.\n", what_time_is_it_now()-curr_time);
    prediction_time = (what_time_is_it_now()-curr_time);
    int nboxes = 0;
    curr_time = what_time_is_it_now();
    detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
    if (nms) do_nms_sort(dets, nboxes, meta.classes, nms);
    draw_detections(im, dets, nboxes, thresh, names, alphabet, meta.classes);
    boxing_time = (what_time_is_it_now()-curr_time);
    //printf("Boxes generated in %f seconds.\n", what_time_is_it_now()-curr_time);


    /*************************************************************************************/
    /* Write log and images                                                              */
    /*************************************************************************************/
    time(&timestamp);
    object_detected = false;
    for(i = 0; i < nboxes; ++i){
      for(j = 0; j < meta.classes; ++j) {
	if (dets[i].prob[j]) {
          // Logging to text file
	  fprintf(fp, "%d,%ld,%d,%s,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                                          pipe_id,
                                          timestamp,
                                          coco_ids[j],
					  names[j],
					  dets[i].prob[j],
					  read_time,
					  conversion_time,
					  prediction_time,
					  boxing_time
					  );
	  object_detected = true;
	}
      }
    }
    
    if (object_detected) {
      // We just log images where objects were detected
      snprintf(outfile, 270, "cam_%d_frame_%05ld", pipe_id, count);
      save_image(im, outfile);
    }


    /*************************************************************************************/
    /* Free and release stuff                                                            */
    /*************************************************************************************/
    free_detections(dets, nboxes);
    free_image(im);
    free_image(sized);


    /*************************************************************************************/
    /* Show signs of life                                                                */
    /*************************************************************************************/
    if(access("alive", F_OK ) == 0)
      // File 'alive' exists; delete it
      remove("alive");


    fflush(stdout);
    fflush(stderr);
    fflush(fp);

    count++;
    sleep(1);

  } while (!exit_loop);

  // Flush and close input and output pipes
  close_input_pipes(input);
  free(data);
  free_network(net);
  fclose(fp);

#ifdef NNPACK
  pthreadpool_destroy(net->threadpool);
  nnp_deinitialize();
#endif

  printf("Exiting...\n");
  return 0;
}

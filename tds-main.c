/*
 * Copyright 2022 IBM
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
#include "utils/microjson-1.6/mjson.h"
#include "cv_toolset.h"

#define FFPROBE_CMD "ffprobe -v error -show_entries stream=width,height -of default=noprint_wrappers=1:nokey=1 %s"
//#define FFMPEG_CMD  "ffmpeg -hide_banner -loglevel error -rtsp_transport tcp -i %s -filter:v fps=0.25 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -"
//#define FFMPEG_CMD  "ffmpeg -hide_banner -loglevel error -i %s -filter:v fps=0.25 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -"
#define FFMPEG_CMD "ffmpeg -hide_banner -loglevel error -r 60 -i %s -r 0.25 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -"
#define CAMS 6
#define CATEGS 80

const char *build_str = "This build was compiled at " __DATE__ ", " __TIME__ ".";

bool exit_loop      = false;
unsigned int tds_id = 0;

typedef struct {
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


void print_usage(char * pname)
{
  printf("Usage: %s <OPTIONS>\n", pname);
  printf(" OPTIONS:\n");
  printf("    -h          : print this helpful usage info\n");
  printf("    -c <file>   : JSON configuration file to use\n");
  printf("    -d <dir>    : directory for this run (i.e. where all logs and output will be placed)\n");
  printf("                :      Optional (default: current directory)\n");
  printf("    -l <file>   : global log file where we append the classification results during the run\n");
  printf("                :      Optional (default: no global logging)\n");
  printf("    -i <id>     : integer id to assign to this run\n");
  printf("                :      Optional (default: 0)\n");
}


void to_json_string(short sequence[CAMS][CATEGS], char *sequence_str)
{
  time_t timestamp;
  int cam, categ;
  char tmp[10];

  time(&timestamp);

  sprintf(sequence_str, "{\"id\":%d,\"ts\":%ld,", tds_id, timestamp);
  for (cam=0; cam<CAMS; cam++) {
    strcat(sequence_str, "\"");
    sprintf(tmp, "%d", cam+1);
    strcat(sequence_str, tmp);
    strcat(sequence_str, "\":[");
    for (categ=0; categ<CATEGS; categ++)
      if (sequence[cam][categ] != -1) {
        //strcat(sequence_str, "\"");
        sprintf(tmp, "%d", sequence[cam][categ]);
        strcat(sequence_str, tmp);
        strcat(sequence_str, ",");
        //strcat(sequence_str, "\",");
      }
    if (sequence_str[strlen(sequence_str)-1] == ',')
      sequence_str[strlen(sequence_str)-1] = '\0';  // Remove last comma
    strcat(sequence_str, "],");
  }
  sequence_str[strlen(sequence_str)-1] = '\0';  // Remove last comma
  strcat(sequence_str, "}");
}


double what_time_is_it_now()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}


int main(int argc, char *argv[])
{

  char confile[256];
  char dirname[256];
  char logfile[256];
  confile[0] = '\0';
  logfile[0] = '\0';
  strcpy(dirname, ".");
  int option;

  printf("------------------------------------------------------------------------------------\n");
  printf(" Starting TDS\n");
  printf(" %s\n", build_str);
  printf("------------------------------------------------------------------------------------\n\n");
  fflush(stdout);

  while ((option = getopt(argc, argv, ":hc:d:l:i:")) != -1) {
    switch(option) {
      case 'h':
        print_usage(argv[0]);
        exit(0);
      case 'c':
	snprintf(confile, 255, "%s", optarg);
	break;
      case 'd':
	snprintf(dirname, 255, "%s", optarg);
	break;
      case 'l':
	snprintf(logfile, 255, "%s", optarg);
	break;
      case 'i':
	tds_id = atoi(optarg);
	break;
      case ':':
	printf("Option %c needs a value\n", optopt);
	exit(-1);
      case '?':
	printf("Unknown option: %c\n", optopt);
	exit(-1);
    }
  }

  if (confile[0] == '\0') {
    printf("ERROR: a JSON configuration file was not specified\n");
    print_usage(argv[0]);
    exit(-1);
  }

  FILE *fp_log = NULL;
  if (logfile[0] != '\0')
    fp_log = fopen(logfile, "a");

  signal(SIGINT, sig_handler);


  /*************************************************************************************/
  /* Create sub-directory for this specific run (we chdir later in the code below)     */
  /*************************************************************************************/
  // Check if directory exists
  DIR* dir = opendir(dirname);
  if (dir)
    // Directory exists
    closedir(dir);
  else if (ENOENT == errno) {
    // Directory does not exist
    printf("ERROR: directory %s doesn't exist\n", dirname);
    exit(-1);
  }


  /*************************************************************************************/
  /* Parse JSON configuration file                                                     */
  /*************************************************************************************/
  conf_params_t conf_params;
  if (parse_config_file(confile, &conf_params) != 0) {
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

  printf("PID:            %d\n", getpid());
  printf("TDS instance:   %d\n", tds_id);
  printf("Run directory:  %s\n", dirname);
  printf("FFmpeg command: %s\n", FFMPEG_CMD);
  printf("\n");
  fflush(stdout);


  // This is the initialization of the PyTorch Tiny-YOLOv2 model
  if (cv_toolset_init() != 0) {
      printf("Computer Vision toolset initialization failed...\n");
      exit(1);
  }

  double curr_time;



  /*************************************************************************************/
  /* Create pipe to read from video/image source                                       */
  /*************************************************************************************/
  input_t input;
  if (open_input_pipes(conf_params, &input) != 0){
    printf("ERROR: cannot open input pipe(s)\n");
    exit(-1);
  }

  unsigned char *data = malloc(sizeof(unsigned char)*dimensions.width*dimensions.height*dimensions.c);
  unsigned long count = 0, n = 0;

  // Some statistics
  double read_time       = 0;
  double conversion_time = 0;
  double prediction_time = 0;
  double boxing_time     = 0;

  FILE *pipein;
  int cam_id;
  char filename[300];
  time_t timestamp;
  int read_attempt = 0;
  short sequence[CAMS][CATEGS];
  char sequence_str[3000];
  bool first_time = true;

  chdir(dirname);
  FILE *fp_pred = fopen("predictions.log", "w");
  fprintf(fp_pred, "cam_id,time,object_id,object_name,prob,read_time_sec,conv_time_sec,pred_time_sec,bbox_time_sec\n");

  do {

    /*************************************************************************************/
    /* Show signs of life                                                                */
    /*************************************************************************************/
    if(access("alive", F_OK ) == 0)
      // File 'alive' exists; delete it
      remove("alive");


    /*************************************************************************************/
    /* Capture image to process                                                          */
    /*************************************************************************************/
    cam_id = (n % CAMS) + 1;

    if (cam_id == 1) {
      // We start a new camera "sequence" (cam1, cam2, cam3, cam4, cam5, cam6). We keep all
      // the classification results for a given sequence together for logging convenience

      // First dump the previous sequence to the global logfile (if specified) in JSON format
      if (fp_log != NULL && !first_time) {
        to_json_string(sequence, sequence_str);
        fprintf(fp_log, "%s\n", sequence_str);
        fflush(fp_log);
      }
      // We start the new sequence
      int cam, categ;
      for (cam=0; cam<CAMS; cam++)
	for (categ=0; categ<CATEGS; categ++)
	  sequence[cam][categ] = -1;
      first_time = false;
    }

    pipein = get_pipe(cam_id, input); n++;
    if (pipein == NULL) continue;
    printf("Reading from pipe %d (%p)\n", cam_id, (void *)pipein); fflush(stdout);
    curr_time = what_time_is_it_now();
    size_t size = fread(data, 1, dimensions.width*dimensions.height*dimensions.c, pipein);
    read_attempt++;
    //printf("Read %d\n", size); fflush(stdout);
    read_time = (what_time_is_it_now()-curr_time);
    if (size == 0 || size != dimensions.width*dimensions.height*dimensions.c) {
      printf("Warning: %zu bytes read (expected: %d)!\n", size, dimensions.width*dimensions.height*dimensions.c); fflush(stdout);
      if (read_attempt == 30) {
        printf("Tried 30 reading attempts. Now quitting.\n"); fflush(stdout);
        exit_loop = true;
      }
      sleep(1);
      continue;
    }
    read_attempt = 0;


    /*************************************************************************************/
    /* Detect objects                                                                    */
    /*************************************************************************************/

    int nboxes = 0;
    snprintf(filename, 270, "cam_%d_frame_%05ld.jpg", cam_id, count);
    detection_t *dets = run_object_classification(data, dimensions, filename, &nboxes);

    if (dets == NULL)
      printf("run_object_classification failed (skipping this frame)\n");


    /*************************************************************************************/
    /* Write log and images                                                              */
    /*************************************************************************************/

    time(&timestamp);
    int j = 0;
    for (int i = 0; i < nboxes; ++i) {
	fprintf(fp_pred, "%d,%ld,%ld,%s,%.4f,%.4f,%.4f,%.4f,%.4f\n",
		  cam_id,
		  timestamp,
		  dets[i].id,
		  dets[i].class_label,
		  dets[i].confidence,
		  read_time,
		  conversion_time,
		  prediction_time,
		  boxing_time
		);
	sequence[cam_id-1][j++] = dets[i].id;
    }
    

    /*************************************************************************************/
    /* Free and release stuff                                                            */
    /*************************************************************************************/
    fflush(stdout);
    fflush(stderr);
    fflush(fp_pred);
    if (dets != NULL)
      free(dets);

    count++;
    sleep(5);

  } while (!exit_loop);


  // Flush and close input and output pipes
  close_input_pipes(input);
  free(data);
  fclose(fp_pred);

  if (fp_log != NULL) {
    to_json_string(sequence, sequence_str);
    fprintf(fp_log, "%s\n", sequence_str);
    fflush(fp_log);
    fclose(fp_log);
  }

  printf("Exiting...\n");
  return 0;
}

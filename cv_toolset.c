/*
 * Copyright 2022 IBM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <Python.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cv_toolset.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h>

PyObject *pName, *pModule, *pFunc, *pFunc_load;
PyObject *pArgs, *pValue, *pretValue;

char *python_module	= "tiny_yolov2";
char *python_func	= "predict2";
char *python_func_load	= "loadmodel";

PyObject *python_yolo_model;

// To call pickle (serialization) routines when
// we want to send Python objects over sockets
PyObject *module = NULL;


int recv_all(int sock, char *buf, int len)
{
  ssize_t n;

  while (len > 0) {
      n = recv(sock, buf, len, 0);

      if (n <= 0)
	return n;
      buf += n;
      len -= n;
  }

  return 1;
}


int cv_toolset_init() {

  PyObject *module_name, *module, *dict, *python_class;

  printf("In the cv_toolset_init routine\n");

  // Initialize the Python interpreter. In an application embedding Python,
  // this should be called before using any other Python/C API functions
  Py_Initialize();
  import_array();

  // Returns new reference
  module_name = PyUnicode_FromString("tiny_yolov2");

  // Returns new reference
  module = PyImport_Import(module_name);
  Py_DECREF(module_name);
  if (module == NULL) {
      PyErr_Print();
      printf("Fails to import the module. Perhaps PYTHONPATH needs to be set: export PYTHONPATH=<your_tds_dir>/yolo\n");
      return 1;
  }

  // Returns borrowed reference
  dict = PyModule_GetDict(module);
  Py_DECREF(module);
  if (dict == NULL) {
      PyErr_Print();
      printf("Fails to get the dictionary.\n");
      return 1;
  }

  // Returns borrowed reference
  python_class = PyDict_GetItemString(dict, "TinyYOLOv2NonLeaky");
  //Py_DECREF(dict);
  if (python_class == NULL) {
      PyErr_Print();
      printf("Fails to get the Python class.\n");
      return 1;
  }

  // Creates an instance of the class
  if (PyCallable_Check(python_class)) {
      // Returns new reference
      python_yolo_model = PyObject_CallObject(python_class, NULL);
      if (python_yolo_model == NULL) {
          PyErr_Print();
          printf("Fails to create Python object.\n");
          return 1;
      }
      //Py_DECREF(python_class);
  } else {
      printf("Cannot instantiate the Python class.\n");
      //Py_DECREF(python_class);
      return 1;
  }

  // Call a method of the class instance; in this case, the load() method to load the model weights
  // Returns new reference
  PyObject *value = PyObject_CallMethod(python_yolo_model, "load", "(s)", "yolo/yolov2-tiny.weights");
  if (value == NULL) {
      PyErr_Print();
      printf("Fails to call load() method.\n");
      return 1;
  }
  Py_DECREF(value);

#ifdef ENABLE_NVDLA
  // Initialize NVDLA
  initNVDLA();
#endif

  return 0;

}


detection_t *run_object_classification(unsigned char *data, dim_t dimensions, char *filename, int *nboxes) {

  detection_t *detections = NULL;

  if (python_yolo_model != NULL) {

      npy_intp dims[] = {dimensions.height, dimensions.width, dimensions.c};
      // Returns new or borrowed reference?
      PyObject *pValue = PyArray_SimpleNewFromData(3, dims, NPY_UINT8, data);

      if (pValue) {

	  // Returns new reference
	  PyObject *list = PyObject_CallMethod(python_yolo_model, "predict", "Os", pValue, filename);
	  Py_XDECREF(pValue);

	  if (list) {

	      // Here we process the list of dictionaries returned by the Python predict() function,
	      // and convert it into an array of detection_t structs. Each detection_t struct in the
	      // array corresponds to a detected object (and its bounding box) in the image.

	      *nboxes    = (int)PyList_Size(list);
	      detections = (detection_t *)malloc(*nboxes * sizeof(detection_t));

	      for (Py_ssize_t i = 0; i < *nboxes; i++) {

		  // Returns borrowed reference
		  PyObject* dict = PyList_GetItem(list, i);

		  if (!PyDict_Check(dict)) {
		      PyErr_SetString(PyExc_TypeError, "List must contain dictionaries");
		      PyErr_Print();
		      Py_XDECREF(list);
		      return NULL;
		  }

		  PyObject *key, *item;

		  // Returns new reference
		  key = PyUnicode_FromString("class_label");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  const char* class_label = PyUnicode_AsUTF8(item);
		  Py_XDECREF(key);

		  // Returns new reference
		  key = PyUnicode_FromString("id");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  long id = PyLong_AsLong(item);
		  Py_XDECREF(key);

		  // Returns new reference
		  key = PyUnicode_FromString("x_top_left");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  double x_top_left = PyFloat_AsDouble(item);
		  Py_XDECREF(key);

		  // Returns new reference
		  key = PyUnicode_FromString("y_top_left");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  double y_top_left = PyFloat_AsDouble(item);
		  Py_XDECREF(key);

		  // Returns new reference
		  key = PyUnicode_FromString("width");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  double width = PyFloat_AsDouble(item);
		  Py_XDECREF(key);

		  // Returns new reference
		  key = PyUnicode_FromString("height");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  double height = PyFloat_AsDouble(item);
		  Py_XDECREF(key);

		  // Returns new reference
		  key = PyUnicode_FromString("confidence");
		  // Returns borrowed reference
		  item = PyDict_GetItem(dict, key);
		  double confidence = PyFloat_AsDouble(item);
		  Py_XDECREF(key);

		  snprintf(detections[i].class_label, 255, "%s", class_label);
		  detections[i].id          = id;
		  detections[i].x_top_left  = x_top_left;
		  detections[i].y_top_left  = y_top_left;
		  detections[i].width       = width;
		  detections[i].height      = height;
		  detections[i].confidence  = confidence;

	      }
	      Py_XDECREF(list);

	  } else {
	      PyErr_Print();
	      return NULL;
	  }

      } else {
	  PyErr_Print();
	  return NULL;
      }
  }

  return detections;
}


detection_t *run_object_classification_remote(unsigned char *data, dim_t dimensions, char *filename, int *nboxes, int sock) {

  char nboxes_str[11];
  char results_size_str[11];
  int  results_size;
  char ret_code;
  char *saveptr1=NULL, *saveptr2=NULL;
  detection_t *detections = NULL;


  /*************************************************************************************/
  /* 1) We send the JPEG filename                                                      */
  /*************************************************************************************/
  if(send(sock, filename, strlen(filename), 0) != strlen(filename)) {
      printf("ERROR: cannot send filename over socket. %s\n", strerror(errno));
      return NULL;
  }
  if(recv(sock, &ret_code, sizeof(ret_code), 0) != sizeof(ret_code)) {
      printf("ERROR: cannot receive ACK from object recognition server. %s\n", strerror(errno));
      return NULL;
  }

  /*************************************************************************************/
  /* 2) We send the frame dimensions                                                   */
  /*************************************************************************************/
  char dims[50];
  sprintf(dims, "%d,%d,%d", dimensions.height, dimensions.width, dimensions.c);
  if(send(sock, dims, strlen(dims), 0) != strlen(dims)) {
      printf("ERROR: cannot send dimensions over socket. %s\n", strerror(errno));
      return NULL;
  }
  if(recv(sock, &ret_code, sizeof(ret_code), 0) != sizeof(ret_code)) {
      printf("ERROR: cannot receive ACK from object recognition server. %s\n", strerror(errno));
      return NULL;
  }

  /*************************************************************************************/
  /* 3) We send the image (bytes array)                                                */
  /*************************************************************************************/
  int len = dimensions.height * dimensions.width * dimensions.c;
  if(send(sock, data, len, 0) != len) {
      printf("ERROR: cannot send image of size %d bytes over socket. %s\n", len, strerror(errno));
      return NULL;
  }

  /*************************************************************************************/
  /* 4) We receive the classification results                                          */
  /*************************************************************************************/
  if(recv(sock, nboxes_str, 10, 0) <= 0) {
      printf("ERROR: cannot receive the number of bounding boxes from object recognition server. %s\n", strerror(errno));
      return NULL;
  }
  nboxes_str[10] = '\0';
  *nboxes = atoi(nboxes_str);

  if(recv(sock, results_size_str, 10, 0) <= 0) {
      printf("ERROR: cannot receive results size from object recognition server. %s\n", strerror(errno));
      return NULL;
  }
  results_size_str[10] = '\0';
  results_size = atoi(results_size_str);

  char *buffer = (char *)malloc(sizeof(char) * (results_size+1));
  int err = recv_all(sock, buffer, results_size);
  if (err <= 0) {
      printf("ERROR: could not read message. %s\n", strerror(errno));
      return NULL;
  }
  buffer[results_size] = '\0';
  //printf("FROM SERVER: %s\n", buffer);

  detections = (detection_t *)malloc(*nboxes * sizeof(detection_t));

  char* line = strtok_r(buffer, "\n", &saveptr1);
  int i = 0;
  bool header_line = true;

  while (line != NULL) {

      if (header_line) {
	  // Skip the header line
	  header_line = false;
	  line = strtok_r(NULL, "\n", &saveptr1);
	  continue;
      }

      // Parse CSV fields

      //  ,image,class_label,id ,x_top_left      ,y_top_left       ,width          ,height        ,confidence
      // 0,0    ,person     ,1.0,583.037841796875,65.78543090820312,19.244873046875,27.00048828125,0.2906472682952881

      // Skip the first two fields
      char* field = strtok_r(line, ",", &saveptr2);
      field = strtok_r(NULL, ",", &saveptr2);

      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%s", detections[i].class_label);
      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%ld", &(detections[i].id));
      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%lf", &(detections[i].x_top_left));
      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%lf", &(detections[i].y_top_left));
      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%lf", &(detections[i].width));
      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%lf", &(detections[i].height));
      field = strtok_r(NULL, ",", &saveptr2);
      sscanf(field, "%lf", &(detections[i].confidence));

      /*
      printf("class_label: %s\n", detections[i].class_label);
      printf("id:          %ld\n", detections[i].id);
      printf("x_top_left:  %lf\n", detections[i].x_top_left);
      printf("y_top_left:  %lf\n", detections[i].y_top_left);
      printf("width:       %lf\n", detections[i].width);
      printf("height:      %lf\n", detections[i].height);
      printf("confidence:  %lf\n", detections[i].confidence);
      */

      line = strtok_r(NULL, "\n", &saveptr1);
      i++;
  }

  free(buffer);

  return detections;

}

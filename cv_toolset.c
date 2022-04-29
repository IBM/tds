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
#include <Python.h>
#include "cv_toolset.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h>

PyObject *pName, *pModule, *pFunc, *pFunc_load;
PyObject *pArgs, *pValue, *pretValue;

char *python_module	= "tiny_yolov2";
char *python_func	= "predict2";
char *python_func_load	= "loadmodel";

PyObject *object;


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
      object = PyObject_CallObject(python_class, NULL);
      if (object == NULL) {
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
  PyObject *value = PyObject_CallMethod(object, "load", "(s)", "yolo/yolov2-tiny.weights");
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

  if (object != NULL) {

      npy_intp dims[] = {dimensions.height, dimensions.width, dimensions.c};
      // Returns new or borrowed reference?
      PyObject *pValue = PyArray_SimpleNewFromData(3, dims, NPY_UINT8, data);

      if (pValue) {

	  // Returns new reference
	  PyObject *list = PyObject_CallMethod(object, "predict", "Os", pValue, filename);
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

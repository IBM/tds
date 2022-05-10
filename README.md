# Threat Detection System (TDS)

This application implements a simple object/threat detection system using [TinyYoloV2/Lightnet](https://eavise.gitlab.io/lightnet/) and [available weights](https://pjreddie.com/media/files/yolov2-tiny.weights) for datasets like [COCO](https://cocodataset.org/#home).

## Requirements

TDS has been successfully built and executed using the following set-up:
 - Ubuntu 18.04 and 20.04 (using Python 3)

TDS requires:
 - [lightnet](https://eavise.gitlab.io/lightnet/notes/01-installation.html)
 - [ffmpeg](https://www.ffmpeg.org) (installable with `sudo apt install ffmpeg`)
 - python3-dev
 - python3-opencv
 - torch and torchvision
 - pytorch-lightning

## Installation and Execution

### Build and Configuration

The installation and execution are fairly standard:

```
git clone git@github.com:IBM/tds.git
cd tds
make clean
make
```

Set up the `PYTHONPATH` environment variable to point to the TinyYoloV2/Lightnet Python files:

```
export PYTHONPATH=<your_tds_home>/yolo. 
```

Download the [yolov2-tiny.weights](https://pjreddie.com/media/files/yolov2-tiny.weights) weights file into `<your_tds_home>/yolo`.

### Usage

```
./tds -h
Usage: ./tds <OPTIONS>
 OPTIONS:
    -h           : print this helpful usage info
    -c <file>    : JSON configuration file to use
    -d <dir>     : directory for this run (i.e. where all logs and output will be placed)
                 :      Optional (default: current directory)
    -l <file>    : global log file where we append the classification results during the run
                 :      Optional (default: no global logging)
    -i <id>      : integer id to assign to this run
                 :      Optional (default: 0)
    -s <ip_addr> : object recognition server's TCP IP address
    -p <port>    : object recognition server's TCP port
```

`-c` specifies the JSON configuration file to use. So in its simplest form, TDS can be executed with the following command:

```
./tds -c ./conf.json
```

We also need to indicate the input image(s) to classify; for example, by setting the `input_image` field in the JSON configuration file with the path and file name of an image (e.g. [dog.jpg](https://github.com/pjreddie/darknet/blob/master/data/dog.jpg)):

```
"input_image" : "./dog.jpg",
```

In this case, TDS will start, load the weights into the TinyYoloV2/Lightnet model, classify that single image, and exit. Alternativelly, we can classify camera frames by indicating the RTSP URL of up to six IP cameras (using the fields `input_stream_*` in the JSON configuration file).

The example above runs TDS as a single (monolitic) application; i.e. a single process running on a single computer. In some cases, we may want to separate the TDS _frontend_ (interface with the cameras) frpm the TDS _backend_ (object detection model). This is particularly useful when the object detection model is offloaded to a separate unit (e.g. external FPGA accelerator) from the CPU where TDS is launched. To enable this mode, TDS has to be built using the `REMOTE_CLASSIFIER` preprocessor macro:

```
make CFLAGS=-DREMOTE_CLASSIFIER
```

We then start the TDS backend server (object detection model):

```
cd <your_tds_home>/yolo/
python ./tiny_yolov2_coco.py -p 65432
```

The `-p` option indicates the port where the object recognition model socket 'listens' (`65432` is just an example; other port numbers also work).

In a different terminal, we start the TDS frontend (interface with the cameras and logging):

```
cd <your_tds_home>/
./tds -c ./my_conf_local.json -l log.json -s 127.0.0.1 -p 65432
```

The `-s` and `-p` options indicate the IP address and port of the object recognition model server, respectively. In this example, both the frontend and backend run on the same computer, and then we can use the `127.0.0.1` loopback address.

<img src="https://raw.githubusercontent.com/IBM/tds/pytds/diagram.png" width="600">

Finally, we can also indicate what image dataset to target, from the two datasets supported: [COCO](https://cocodataset.org/) and [ATR](https://dsiac.org/databases/atr-algorithm-development-image-database/). To do this, set the `python_module` field in the JSON configuration file appropriately:

```
"python_module" : "tiny_yolov2_coco",
```

or

```
"python_module" : "tiny_yolov2_atr",
```

Similarly, set the `model_weights` field in the JSON configuration file with the corresponding weights file (not included).


## Contributors and Current Maintainers

 * Augusto Vega (IBM) --  [ajvega@us.ibm.com](mailto:ajvega@us.ibm.com)



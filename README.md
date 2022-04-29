# Threat Detection System (TDS)

This application implements a simple object/threat detection system using [TinyYoloV2/Lightnet](https://eavise.gitlab.io/lightnet/) and [available weights](https://pjreddie.com/media/files/yolov2-tiny.weights) for datasets like [COCO](https://cocodataset.org/#home).

## Requirements

TDS has been successfully built and executed using the following set-up:
 - Ubuntu 18.04 and 20.04

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
make -f Makefile.local clean
make -f Makefile.local
```

To run TDS, we first set up `PYTHONPATH` appropriately: `export PYTHONPATH=<your_tds_home>/yolo`. We also need to indicate the input image(s) to classify; for example setting the `input_image` field with the path and file name of an image (e.g. [dog.jpg](https://github.com/pjreddie/darknet/blob/master/data/dog.jpg)):

```
"input_image"        :  "./dog.jpg",
```

In this case, TDS will start, load the weights into the TinyYoloV2/Lightnet model, classify that single image, and exit. Alternativelly, we can classify camera frames by indicating the RTSP URL of up to six IP cameras (using the fields `input_stream_*`).

We also need to download the [yolov2-tiny.weights](https://pjreddie.com/media/files/yolov2-tiny.weights) weights file into `<your_tds_home>/yolo`.

### Usage

```
./tds -h
Usage: ./tds <OPTIONS>
 OPTIONS:
    -h          : print this helpful usage info
    -c <file>   : JSON configuration file to use
    -d <dir>    : directory for this run (i.e. where all logs and output will be placed)
                :      Optional (default: current directory)
    -l <file>   : global log file where we append the classification results during the run
                :      Optional (default: no global logging)
    -i <id>     : integer id to assign to this run
                :      Optional (default: 0)
```

`-c` is the only mandatory argument, which specifies the JSON configuration file to use. So in its simplest form, TDS can be executed with the following command:

```
./tds -c ./conf.json
```


## Contributors and Current Maintainers

 * Augusto Vega (IBM) --  [ajvega@us.ibm.com](mailto:ajvega@us.ibm.com)



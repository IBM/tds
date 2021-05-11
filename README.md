# Threat Detection System (TDS)

This application implements a simple object/threat detection system using [YOLOv3/Darknet](https://pjreddie.com/darknet/yolo/) and available weights for datasets like [COCO](https://cocodataset.org/#home).

## Requirements

TDS has been successfully built and executed using the following set-up:
 - Ubuntu 18.04 and 20.04
 - Raspbian GNU/Linux 10 (buster) (Raspberry Pi platform)

TDS for Ubuntu/x86 requires:
 - [darknet](https://pjreddie.com/darknet/yolo/)
 - [ffmpeg](https://www.ffmpeg.org) (installable with `sudo apt install ffmpeg`)

TDS for Raspberry Pi requires:
 - [darknet-nnpack](https://github.com/digitalbrain79/darknet-nnpack) (a good tutorial [here](https://egemenertugrul.github.io/blog/Darknet-NNPACK-on-Raspberry-Pi/))
 - [ffmpeg](https://www.ffmpeg.org) (installable with `sudo apt install ffmpeg`)
 
We really do not need OpenCV for this version of TDS; so it is safe to skip instructions related to OpenCV installation :)

**Static linking**: In the specific Raspberry Pi case, for convenience we build the application _statically_ (`-static`). Because of this, we also need to statically build `nnpack` (`nnpack`, not `darknet-nnpack`) using the following command:

```
cmake -G Ninja -DNNPACK_LIBRARY_TYPE=static ..
```

This generates `libnnpack.a` and `libpthreadpool.a`, which are included during linking time of the `tds` executable.


## Usage

The installation and execution are fairly standard. For Ubuntu/x86 systems:

```
git clone git@github.com:IBM/tds.git
cd tds
make -f Makefile.local clean
make -f Makefile.local
```

For Raspberry Pi systems:

```
git clone git@github.com:IBM/tds.git
cd tds
make -f Makefile.rpi clean
make -f Makefile.rpi
```

There is no fundamental difference between building on Ubuntu/x86 versus Raspberry Pi systems, except for the YOLOv3/Darknet version used (`darknet` or `darknet-nnpack`). This is why we employ two different makefiles (`Makefile.local` and `Makefile.rpi`).

To run TDS we first need to setup its JSON configuration file to indicate paths related to the `darknet` (or `darknet-nnpack`) installation using the `darknet_*` fields. We also need to indicate the input image(s) to classify; for example setting the `input_image` field with the path and file name of an image (e.g. [dog.jpg](https://github.com/pjreddie/darknet/blob/master/data/dog.jpg)):

```
"input_image"        :  "./dog.jpg",
```

In this case, TDS will start, load the weights into the YOLOv3/Darknet model, classify that single image, and exit. Alternativelly, we can classify camera frames by indicating the RTSP URL of up to six IP cameras (using the fields `input_stream_*`).

We also need to create a soft link to the YOLOv3/Darknet `data` folder in our TDS home directory. This will allow YOLOv3/Darknet to find some additional required files:

```
cd tds/
ln -s <darknet-home>/data/
```

Finally, we can run TDS:

```
./tds ./conf.json
```



## Contributors and Current Maintainers

 * Augusto Vega (IBM) --  [ajvega@us.ibm.com](mailto:ajvega@us.ibm.com)



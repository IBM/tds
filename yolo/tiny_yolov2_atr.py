import sys, getopt
import socket
import struct
import cv2
import torch
import torch.nn as nn
import torchvision
import lightnet as ln
import pytorch_lightning as pl
import lightnet.network as lnn
from loss_func import RegionLoss
from collections import OrderedDict, Iterable
from torchvision import transforms
from IPython.display import display
from PIL import Image, ImageDraw
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd


BOX_COLOR = (255, 0, 0) # Red
TEXT_COLOR = (255, 255, 255) # White

classes = ['BMP2', 'D20', 'MTLB', 'T72', 'ZSU23', '2S3',
           'PICKUP', 'MAN', 'VEHICLE', 'SUV', 'BRDM2', 'BTR70']


class LITinyYolo(pl.LightningModule):
    def __init__(self, classes, map_style="coco",
                stride=32, 
                anchors=[(1.08, 1.19), (3.42, 4.41), (6.63, 11.38), (9.42, 5.11), (16.62, 10.52)]):
        super().__init__()
        self.map_style = map_style
        self.classes = classes
        self.anchors = anchors
        self.stride = stride
        self.network = TinyYOLOv2NonLeaky(classes)
        self.loss = RegionLoss(
            num_classes=len(classes),
            anchors=self.network.anchors,
            stride=self.network.stride
        )
    
    def forward(self, image):
        prediction = self.network(image)
        return prediction
    

class TinyYOLOv2NonLeaky(lnn.module.Darknet):
    stride = 32
    inner_stride = 32
    remap_darknet = [
        (r'^layers.0.(\d+_)',   r'layers.\1'),  # All base layers (1-13)
    ]

    def __init__(self,
                 classes = classes,
                 input_channels=3,
                 anchors=[(0.57273, 0.677385), (1.87446, 2.06253), (3.33843, 5.47434), (7.88282, 3.52778), (9.77052, 9.16828)]):

        super().__init__()
        if not isinstance(anchors, Iterable) and not isinstance(anchors[0], Iterable):
            raise TypeError('Anchors need to be a 2D list of numbers')

        # Parameters
        self.classes        = classes
        self.input_channels = input_channels
        self.anchors        = anchors

        # Network
        momentum = 0.01
        self.layers = nn.Sequential(
            OrderedDict([
                ('1_convbatch',     lnn.layer.Conv2dBatchReLU(input_channels, 16, 3, 1, 1, momentum=momentum)),
                ('2_max',           nn.MaxPool2d(2, 2)),
                ('3_convbatch',     lnn.layer.Conv2dBatchReLU(16, 32, 3, 1, 1, momentum=momentum)),
                ('4_max',           nn.MaxPool2d(2, 2)),
                ('5_convbatch',     lnn.layer.Conv2dBatchReLU(32, 64, 3, 1, 1, momentum=momentum)),
                ('6_max',           nn.MaxPool2d(2, 2)),
                ('7_convbatch',     lnn.layer.Conv2dBatchReLU(64, 128, 3, 1, 1, momentum=momentum)),
                ('8_max',           nn.MaxPool2d(2, 2)),
                ('9_convbatch',     lnn.layer.Conv2dBatchReLU(128, 256, 3, 1, 1, momentum=momentum)),
                ('10_max',          nn.MaxPool2d(2, 2)),
                ('11_convbatch',    lnn.layer.Conv2dBatchReLU(256, 512, 3, 1, 1, momentum=momentum)),
                ('12_max',          lnn.layer.PaddedMaxPool2d(2, 1, (0, 1, 0, 1))),
                ('13_convbatch',    lnn.layer.Conv2dBatchReLU(512, 1024, 3, 1, 1, momentum=momentum)),
                ('14_convbatch',    lnn.layer.Conv2dBatchReLU(1024, 1024, 3, 1, 1, momentum=momentum)),
                ('15_conv',         nn.Conv2d(1024, len(self.anchors)*(5+len(self.classes)), 1, 1, 0)),
            ])
        )
    
    # We need to provide a specific implementation of load() to be able to
    # load the weights from a PyTorch Lightning checkpoint file (*.ckpt)
    def load(self, weights_file):
        model = LITinyYolo.load_from_checkpoint(weights_file, classes=classes)
        self = model        
        
        
    # this is supposed to be replaced by a C version at some point ??
    # this could also be replaced with non_eval_post_processing which
    # i believe just leaves out the conversion to a pandas dataframe w/
    # typical brambox info
    def postprocessing(self, predictions):
        return ln.data.transform.Compose([
            # GetBoxes transformation generates bounding boxes from network output
            ln.data.transform.GetDarknetBoxes(
                conf_thresh=0.05,  # It was 0.5
                network_stride=self.stride,
                anchors=self.anchors
            ),

            # Filter transformation to filter the output boxes
            ln.data.transform.NMS(
                nms_thresh=0.5
            ),

            # Miscellaneous transformation that transforms the output boxes to a brambox dataframe
            ln.data.transform.TensorToBrambox(
                class_label_map=self.classes,
            )
        ])(predictions)
    
    
    # This is to visualize the bounding box and label on the image
    def draw_single_bbox(self, img, bbox, class_name, color=BOX_COLOR, thickness=2):
        """Adds a single bounding box on the image"""
        x_min, y_min, w, h = bbox
        x_min, x_max, y_min, y_max = int(x_min), int(x_min + w), int(y_min), int(y_min + h)
       
        cv2.rectangle(img, (x_min, y_min), (x_max, y_max), color=color, thickness=thickness)
        
        ((text_width, text_height), _) = cv2.getTextSize(class_name, cv2.FONT_HERSHEY_SIMPLEX, 0.35, 1)    
        cv2.rectangle(img, (x_min, y_min - int(1.3 * text_height)), (x_min + text_width, y_min), BOX_COLOR, -1)
        cv2.putText(
            img,
            text=class_name,
            org=(x_min, y_min - int(0.3 * text_height)),
            fontFace=cv2.FONT_HERSHEY_SIMPLEX,
            fontScale=0.35, 
            color=TEXT_COLOR, 
            lineType=cv2.LINE_AA,
        )
        return img
    
    
    # This is to visualize the bounding boxes and labeles on the image
    def draw_bboxes(self, image, detections):
        img = image.copy()
        detections = detections.reset_index()
        for index, row in detections.iterrows():
            bbox       = [row['x_top_left'], row['y_top_left'], row['width'], row['height']]
            class_name = row['class_label']
            img        = self.draw_single_bbox(img, bbox, class_name)
        return img


    def predict(self, im, filename):

        transform     = transforms.Compose([transforms.ToTensor()])
        input_tensor  = transform(im.astype(np.uint8)).to('cpu').unsqueeze(0)
        output_tensor = lnn.module.Darknet.forward(self, input_tensor)
        output_df     = self.postprocessing(output_tensor)

        im_bboxes = self.draw_bboxes(im, output_df)
        if (len(output_df) > 0):
            # Something was detected. We save the image.
            plt.imsave(filename, im_bboxes)
            
        #print("Plotting image with bounded boxes")
        #plt.figure(figsize=(12, 12))
        #plt.axis('off')
        #plt.imshow(im_bboxes)
        #plt.show()

        # We return the pandas DataFrame (output_df) converted to a list of
        # dcitionaries to facilitate its consumption on the C domain.
        return output_df.to_dict(orient='records')


def main(argv):
    
    port  = ''
    
    if len (argv) > 3:
        print('Usage: ' + argv[0] + ' -p <socket_port>')
        sys.exit(1)
        
    try:
        opts, args = getopt.getopt(argv[1:], "hp:", ["help","port="])
    except getopt.GetoptError:
        print('Usage: ' + argv[0] + ' -p <socket_port>')
        sys.exit(1)
    
    for opt, arg in opts:
        if opt == '-h':
            print('Usage: ' + argv[0] + ' -p <socket_port>')
            sys.exit(0)
        elif opt in ("-p", "--port"):
            port = arg
    
    if (not port):
        print('Usage: ' + argv[0] + ' -p <socket_port>')
        sys.exit(1)
            
    model = TinyYOLOv2NonLeaky()
    model.load('yolov2-tiny.weights')
    
    HOST_IP = socket.gethostbyname(socket.gethostname())
    PORT    = int(port) #65432  # Port to listen on (non-privileged ports are > 1023)

    print('Listening for inbound connections on ' + str(HOST_IP) + ':' + str(PORT))

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        
        s.bind((HOST_IP, PORT))
        s.listen()
        conn, addr = s.accept()
        
        with conn:
            
            print(f"Connected by {addr}")
            
            while True:

                #try:
                # Get JPEG image filename
                filename = conn.recv(1024)
                if not filename:
                    break
                filename = filename.decode("utf-8")
                #print('Received: ', filename)
                conn.send('1'.encode())

                # Get and parse dimensions
                dims = conn.recv(1024)
                if not dims:
                    break
                dims = dims.decode("utf-8")
                dims = [int(x) for x in dims.split(',')]
                #print('Received: ', dims)
                conn.send('2'.encode())

                # Get image
                length = dims[0] * dims[1] * dims[2]
                image = b''
                while len(image) < length:
                    to_read = length - len(image)
                    image += conn.recv(4096 if to_read > 4096 else to_read)
                image = np.frombuffer(image, dtype=np.uint8)
                image = image.reshape(dims)
                output_df = model.predict(image, filename)
                output_df_csv = pd.DataFrame.from_records(output_df).to_csv() 

                # Send the number of bounding boxes (rows in the output_df DataFrame)
                conn.send(str(len(output_df)).ljust(10).encode())

                # Send the size (in bytes) of the output_df DataFrame
                conn.send(str(len(output_df_csv)).ljust(10).encode())
                
                # Send the actual DataFrame in CSV format
                conn.sendall(output_df_csv.encode())    

                #finally:
                #    conn.shutdown(socket.SHUT_WR)
                #    conn.close()


if __name__ == "__main__":
   main(sys.argv)

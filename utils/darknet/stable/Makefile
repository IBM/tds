# compile here + link later with tds
CC = cc
CFLAGS = -O3

all: activate.o batchnorm.o blas.o box.o convolution.o image.o list.o maxpool.o network.o options.o parser.o route.o upsample.o utils.o yolo.o

activate.o:
	$(CC) $(CFLAGS) -c activate.c
batchnorm.o:
	$(CC) $(CFLAGS) -c batchnorm.c
blas.o:
	$(CC) $(CFLAGS) -c blas.c
box.o:
	$(CC) $(CFLAGS) -c box.c
convolution.o:
	$(CC) $(CFLAGS) -c convolution.c
image.o:
	$(CC) $(CFLAGS) -c image.c
list.o:
	$(CC) $(CFLAGS) -c list.c
maxpool.o:
	$(CC) $(CFLAGS) -c maxpool.c
network.o:
	$(CC) $(CFLAGS) -c network.c
options.o:
	$(CC) $(CFLAGS) -c options.c
parser.o:
	$(CC) $(CFLAGS) -c parser.c
route.o:
	$(CC) $(CFLAGS) -c route.c
upsample.o:
	$(CC) $(CFLAGS) -c upsample.c
utils.o:
	$(CC) $(CFLAGS) -c utils.c
yolo.o:
	$(CC) $(CFLAGS) -c yolo.c

clean:
	rm -f *.o

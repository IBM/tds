# compile here + link later with tds
CC = gcc
CFLAGS = -O3 -Wno-unused-result
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(OBJS)

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o

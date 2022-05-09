CC = gcc
override CFLAGS += -I. -I/usr/include/python3.8 -pedantic -Wall -O3
LDFLAGS = -lpython3.8
DEPS =
OBJ = tds-main.o cv_toolset.o
MJSONDIR = utils/microjson-1.6

all: $(MJSONDIR) tds

$(MJSONDIR):
	$(MAKE) -C $@ $(MAKECMDGOALS)
        
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tds: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJSONDIR)/mjson.o

.PHONY: all clean $(MJSONDIR)

clean:
	rm -f *.o tds
	$(MAKE) -C $(MJSONDIR) clean
 
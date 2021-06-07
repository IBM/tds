CC = gcc
CFLAGS = -pedantic -Wall -O3
LDFLAGS = -lm
DEPS =
OBJ = tds-main.o
MJSONDIR = utils/microjson-1.6
DNETDIR = utils/darknet/stable

all: $(MJSONDIR) $(DNETDIR) tds

$(MJSONDIR):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(DNETDIR):
	$(MAKE) -C $@ $(MAKECMDGOALS)
        
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tds: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJSONDIR)/mjson.o $(DNETDIR)/*.o

.PHONY: all clean $(MJSONDIR) $(DNETDIR)

clean:
	rm -f *.o tds
	$(MAKE) -C $(MJSONDIR) clean
	$(MAKE) -C $(DNETDIR) clean
 

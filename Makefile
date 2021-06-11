CC = gcc
CFLAGS = -pedantic -O3 -Wno-unused-result
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
	$(CC) -o $@ $^ $(CFLAGS) $(MJSONDIR)/mjson.o $(DNETDIR)/*.o $(LDFLAGS)

.PHONY: all clean $(MJSONDIR) $(DNETDIR)

clean:
	rm -f *.o tds
	$(MAKE) -C $(MJSONDIR) clean
	$(MAKE) -C $(DNETDIR) clean
 

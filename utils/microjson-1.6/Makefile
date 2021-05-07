# Makefile for the microjson project

# The version for release is derived from the mostt recent stanza in the news file.
VERSION=$(shell sed -n <NEWS.adoc '/::/s/^\([0-9][^:]*\).*/\1/p' | head -1)

CFLAGS = -O

# Add DEBUG_ENABLE for the tracing code
CFLAGS += -DDEBUG_ENABLE -g
# Add TIME_ENABLE to support RFC3339 time literals
CFLAGS += -DTIME_ENABLE

all: mjson.o test_microjson example1 example2 example3 example4

mjson.o: mjson.c mjson.h

test_microjson: test_microjson.o mjson.o
	$(CC) $(CFLAGS) -o test_microjson test_microjson.o mjson.o

test_microjson_wignore: test_microjson_wignore.o mjson.o
	$(CC) $(CFLAGS) -o test_microjson_wignore test_microjson_wignore.o mjson.o

.SUFFIXES: .html .adoc .3

# Requires asciidoc and xsltproc/docbook stylesheets.
.adoc.html:
	asciidoc $*.adoc
.adoc.3:
	a2x --doctype manpage --format manpage $*.adoc

# Regression test
check: test_microjson test_microjson_wignore
	./test_microjson
	./test_microjson_wignore

# Worked examples.  These are essentially subsets of the regression test.
example1: example1.c mjson.c mjson.h
example2: example2.c mjson.c mjson.h
example3: example3.c mjson.c mjson.h
example4: example4.c mjson.c mjson.h

clean:
	rm -f mjson.o test_microjson.o test_microjson test_microjson_wignore example[1234]
	rm -f microjson.html mjson.html

version:
	@echo $(VERSION)

SUPPRESSIONS = --suppress=unusedStructMember --suppress=unreadVariable
SUPPRESSIONS += -U__UNUSED__
cppcheck:
	cppcheck -I. --template gcc --enable=all $(SUPPRESSIONS) *.[ch]

SOURCES = Makefile *.[ch]
DOCS = README.adoc COPYING NEWS.adoc control microjson.adoc mjson.adoc
ALL =  $(SOURCES) $(DOCS)
microjson-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:microjson-$(VERSION)/:' --show-transformed-names -cvzf microjson-$(VERSION).tar.gz $(ALL)

dist: microjson-$(VERSION).tar.gz

release: microjson-$(VERSION).tar.gz microjson.html mjson.html
	shipper version=$(VERSION) | sh -e -x

refresh: microjson.html mjson.html
	shipper -N -w version=$(VERSION) | sh -e -x

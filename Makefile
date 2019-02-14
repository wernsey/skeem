CC=gcc
AWK=awk

# TODO: You need to comment out this line once things get relatively stable
BUILD=debug

CFLAGS = -c -Wall
LDFLAGS =

# Add your source files here:
SOURCES=skeem.c refcnt.c main.c

EXECUTABLE=skeem
DISTFILE=skeem.zip

OBJECTS=$(SOURCES:.c=.o)

ifeq ($(BUILD),debug)
# Debug
#CFLAGS += -O0 -g --std=c11 -pedantic
CFLAGS += -O0 -g
LDFLAGS +=
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
endif

ifeq ($(OS),Windows_NT)
  EXECUTABLE:=$(EXECUTABLE).exe
endif

all: $(EXECUTABLE) docs

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Add header dependencies here
skeem.o : skeem.c skeem.h refcnt.h
refcnt.o : refcnt.c refcnt.h

docs: docsdir docs/skeem.html docs/README.html

docsdir:
	-mkdir docs

docs/skeem.html: skeem.h skeem.c d.awk
	$(AWK) -v Title="API Documentation" -f d.awk skeem.h skeem.c > $@

docs/README.html: README.md d.awk
	$(AWK) -f d.awk -v Clean=1 -v Title="README" $< > $@

.PHONY : clean

clean:
	-rm -f $(EXECUTABLE) $(DISTFILE)
	-rm -f *.o
	-rm -rf docs

dist: clean
	zip $(DISTFILE) *.c *.h Makefile Readme.md d.awk test/*.scm

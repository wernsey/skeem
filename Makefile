CC=gcc

# TODO: You need to comment out this line once things get relatively stable
BUILD=debug

CFLAGS = -c -Wall
LDFLAGS =

# Add your source files here:
SOURCES=skeem.c refcnt.c main.c

OBJECTS=$(SOURCES:.c=.o)

DISTFILE=skeem.zip

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

all: skeem.exe

debug:
	make BUILD=debug

skeem.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Add header dependencies here
skeem.o : skeem.c skeem.h refcnt.h
refcnt.o : refcnt.c refcnt.h

.PHONY : clean

clean:
	-rm -f *.exe $(DISTFILE)
	-rm -f *.o

dist: clean
	zip $(DISTFILE) *.c *.h Makefile Readme.md test/*.scm
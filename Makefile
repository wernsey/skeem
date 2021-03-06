CC=gcc
AWK=awk

# Run `make debug` for a debug build
# BUILD=debug

CFLAGS = -c -Wall
LDFLAGS = -lm

# Add your source files here:
SOURCES=skeem.c main.c

EXECUTABLE=skeem
DOCSDIR = ./doc
DISTFILE=skeem.zip

ifeq ($(BUILD),debug)
# Debug
#CFLAGS += -O0 -g --std=c11 -pedantic
CFLAGS += -O0 -g -DSK_USE_EXTERNAL_REF_COUNTER
LDFLAGS +=
SOURCES += refcnt.c
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
endif

OBJECTS=$(SOURCES:.c=.o)

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
skeem.o : skeem.c skeem.h
refcnt.o : refcnt.c refcnt.h

docs: $(DOCSDIR) $(DOCSDIR)/skeem.html $(DOCSDIR)/Readme.html

$(DOCSDIR):
	-mkdir -p $(DOCSDIR)

$(DOCSDIR)/skeem.html: skeem.h skeem.c d.awk
	$(AWK) -v Title="API Documentation" -f d.awk skeem.h skeem.c > $@

$(DOCSDIR)/Readme.html: Readme.md d.awk
	$(AWK) -f d.awk -v Clean=1 -v Title="README" $< > $@

.PHONY : clean docsdir

clean:
	-rm -f $(EXECUTABLE) $(DISTFILE)
	-rm -f *.o
	-rm -rf $(DOCSDIR)

dist: clean
	zip $(DISTFILE) *.c *.h Makefile *.md d.awk test/*.scm

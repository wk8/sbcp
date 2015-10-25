PREFIX ?= /usr/local

HEADER_FILES = $(wildcard *.h)
OBJECT_FILES = $(subst .h,.o,$(HEADER_FILES))

GCC      = gcc
CFLAGS   = -Wall
OPTFLAGS = -O4

all: sbcp

sbcp: sbcp.o $(OBJECT_FILES)
	$(GCC) $(CFLAGS) $(OPTFLAGS) $^ -o $@

%.o: %.c $(HEADER_FILES)
	$(GCC) -c $(CFLAGS) $(OPTFLAGS) $<

install: sbcp
	cp -f sbcp $(PREFIX)/bin/sbcp

uninstall:
	rm -f $(PREFIX)/bin/sbcp

clean:
	rm -rvf *.o sbcp

.PHONY: all clean install uninstall

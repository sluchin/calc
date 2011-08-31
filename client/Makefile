# Makefile
# $Id$

srcdir = .
top_srcdir = ..
lib_dir = ../../lib
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES = -I$(top_srcdir)/lib
CFLAGS += -g -Wall -O2 -D_USE_SELECT
LDFLAGS =
LIBS = #-lpthread
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(CFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalc.a
PROGRAM = calcc
OBJECTS = main.o \
          option.o \
          client.o

.PHONY: all debug strip clean
.SUFFIXES: .c .o

all: $(PROGRAM) $(OBJECTS)

$(PROGRAM): $(OBJECTS) $(LIBRARY)
	@$(RM) $(PROGRAM)
	$(LINK) -o $@ $^ $(LIBS) $(LDFLAGS)

.c.o:
	cd $(top_srcdir)/lib && $(MAKE)
	cd $(top_srcdir)/client
	$(COMPILE) -c $<

$(OBJECTS): option.h client.h version.h Makefile

debug:
	$(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG -D_USE_SELECT" 

strip:
	$(STRIP) $(PROGRAM)

clean:
	$(RM) $(PROGRAM) $(OBJECTS) data.o


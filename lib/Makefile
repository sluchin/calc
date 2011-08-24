# Makefile
# $Id$

srcdir = .
top_srcdir = ..
lib_dir = ../../lib
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES = -I$(lib_dir)/include
CFLAGS += -g -Wall -O2
LDFLAGS =
LIBS = -lpthread
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(CFLAGS)
LIBRARY = libcalc.a
OBJECTS = data.o \
          util.o \
          log.o \

.PHONY: all debug strip clean

all: $(OBJECTS) $(LIBRARY)
.SUFFIXES: .c .o

$(LIBRARY): $(OBJECTS)
	@$(RM) $(LIBRARY)
	$(AR) $@ $^

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): data.h util.h log.h Makefile

debug:
	$(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 
strip:
	$(STRIP) $(OBJECTS)
clean:
	$(RM) $(OBJECTS) $(LIBRARY)


# Makefile
# $Id$

srcdir = .
top_srcdir = ..
lib_dir = ../../lib
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/calc
CFLAGS += -g -Wall -O2
LDFLAGS =
LIBS = -lpthread -lm
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(CFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalc.a \
          $(top_srcdir)/calc/function.o \
          $(top_srcdir)/calc/error.o \
          $(top_srcdir)/calc/calc.o
PROGRAM = calcd
OBJECTS = main.o \
          option.o \
          server.o

.PHONY: all debug strip clean

all: $(PROGRAM) $(OBJECTS)
.SUFFIXES: .c .o

$(PROGRAM): $(OBJECTS) $(LIBRARY)
	@$(RM) $(PROGRAM)
	$(LINK) -o $@ $^ $(LIBS) $(LDFLAGS)

.c.o:
	cd $(top_srcdir)/lib && $(MAKE)
	cd $(top_srcdir)/calc && $(MAKE)
	cd $(top_srcdir)/server
	$(COMPILE) -c $<

$(OBJECTS): option.h server.h version.h Makefile

debug:
	$(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 

strip:
	$(STRIP) $(PROGRAM)

clean:
	$(RM) $(PROGRAM) $(OBJECTS) calc.o


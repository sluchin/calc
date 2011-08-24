# Makefile
# $Id$

srcdir = .
top_srcdir = ..
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES = -I$(top_srcdir)/lib
CFLAGS += -g -Wall -O2 -D_DEBUG
LDFLAGS =
LIBS = -lm
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(CFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalc.a
PROGRAM = calcp
OBJECTS = main.o \
          option.o \
          function.o \
          error.o \
          calc.o

.PHONY: all debug strip clean
.SUFFIXES: .c .o

all: $(PROGRAM) $(OBJECTS)

$(PROGRAM): $(OBJECTS) $(LIBRARY)
	@$(RM) $(PROGRAM)
	$(LINK) -o $@ $^ $(LIBS) $(LDFLAGS)

.c.o:
	cd $(top_srcdir)/lib && $(MAKE)
	cd $(top_srcdir)/calc
	$(COMPILE) -c $<

$(OBJECTS): option.h calc.h version.h Makefile

debug:
	$(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 

strip:
	$(STRIP) $(PROGRAM)

clean:
	$(RM) $(PROGRAM) $(OBJECTS) test_result


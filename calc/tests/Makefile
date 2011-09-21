# Makefile
# $Id$

srcdir = ..
top_srcdir = ../..
libcalcdir = $(top_srcdir)/lib
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES = -I$(srcdir) -I$(libcalcdir)
CFLAGS += -g -Wall -O2 -D_UT
LDFLAGS =  -L$(srcdir) -L$(libcalcdir)
LIBS = -lm -lpthread -lcalcp -lcalcutil
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(srcdir)/libcalcp.a $(libcalcdir)/libcalcutil.a
PROGRAM = test
OBJECTS = test.o

.SUFFIXES: .c .o

.PHONY: all
all: $(PROGRAM) $(OBJECTS)

$(PROGRAM): $(OBJECTS)
	@$(RM) $(PROGRAM)
	$(LINK) -o $@ $(LIBS) $^

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): Makefile

.PHONY: debug
debug:
	$(MAKE) CFLAGS="$(CFLAGS) -D_DEBUG"

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJECTS)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... doc"
	@echo "... strip"


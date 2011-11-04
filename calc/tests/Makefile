# Makefile
# $Id$

srcdir = .
pardir = ..
top_srcdir = ../..
libcalcdir = $(top_srcdir)/lib
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
CUTTER = /usr/bin/cutter -v v
INCLUDES = -I$(srcdir) -I$(pardir) -I$(libcalcdir) -I/usr/include/cutter
CFLAGS = -g -Wall -O2 -DUNITTEST
DFLAGS = -g -Wall -O2 -DUNITTEST -D_DEBUG
LDFLAGS =  -L$(srcdir) -L$(pardir) -L$(libcalcdir)
LIBS = -lcutter -lcalcutil -lcalcp
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(LDFLAGS)
CALCSOBJ = test_calc.so
FUNCSOBJ = test_function.so
ERRORSOBJ = test_error.so
LIBCALC = $(pardir)/libcalcp.a
CALCOBJ = test_calc.o
FUNCOBJ = test_function.o
ERROROBJ = test_error.o

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(CALCSOBJ) $(FUNCSOBJ) $(ERRORSOBJ)

$(CALCSOBJ): $(CALCOBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS)

$(FUNCSOBJ): $(FUNCOBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS)

$(ERRORSOBJ): $(ERROROBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): Makefile

.PHONY: debug
debug:
	$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test: debug
	$(CUTTER) $(srcdir)

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)

.PHONY: clean
clean:
	$(RM) $(SHAREDOBJ) $(OBJECTS)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... test"
	@echo "... clean"
	@echo "... debug"
	@echo "... strip"
	@echo "... help"



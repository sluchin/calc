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
INCLUDES = -I$(srcdir) -I$(pardir) -I$(libcalcdir) -I/usr/include/cutter
CFLAGS = -g -Wall -O2 -DUNITTEST
DFLAGS = -g -Wall -O2 -DUNITTEST -D_DEBUG
LDFLAGS =  -L$(srcdir) -L$(pardir) -L$(libcalcdir)
LIBS = -lcutter -lcalcutil -lcalcp
TESTLIBS = -ltest_calc
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(LDFLAGS)
CALCSOBJ = test_calc.so
FUNCSOBJ = test_func.so
ERRORSOBJ = test_error.so
CALCOBJ = test_calc.o
FUNCOBJ = test_func.o
ERROROBJ = test_error.o
COMMONOBJ = test_common.o
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(CALCSOBJ) $(FUNCSOBJ) $(ERRORSOBJ)

$(CALCSOBJ): $(CALCOBJ) $(COMMONOBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

$(FUNCSOBJ): $(FUNCOBJ) $(COMMONOBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

$(ERRORSOBJ): $(ERROROBJ) $(COMMONOBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

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
	$(RM) *.so *.o

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... test"
	@echo "... clean"
	@echo "... debug"
	@echo "... strip"
	@echo "... help"


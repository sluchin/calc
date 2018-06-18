# Makefile
# $Id$

srcdir = .
top_srcdir = ..
prefix = /usr/local
libdir = $(prefix)/lib
bindir = $(prefix)/bin
testdir = $(srcdir)/tests
CC = gcc
RM = rm -rf
AR = ar rcs
STRIP = strip
INSTALL = install -c
INCLUDES = -I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/calc
CFLAGS = -g -Wall -O2 -fPIC -DNDEBUG
DFLAGS = -g -Wall -O2 -fPIC -DUNITTEST -D_DEBUG
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib -L$(top_srcdir)/calc
LIBS = -lm -lpthread
UTILLIBS = -lcalcp -lcalcutil
SERVERLIBS = -lcalcd
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a $(top_srcdir)/calc/libcalcp.a
LIBSERVER = libcalcd.a
OBJSERVER = server.o
OBJECTS = main.o option.o
SHAREDOBJ = libcalcd.so
PROGRAM = calcd
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJSERVER) $(LIBSERVER) $(SHAREDOBJ) $(PROGRAM)

$(LIBSERVER): $(OBJSERVER)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJSERVER)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(UTILLIBS) $(LIBS)

$(PROGRAM): $(OBJECTS)
	@$(RM) $(PROGRAM)
	$(LINK) -o $@ $^ $(SERVERLIBS) $(UTILLIBS) $(LIBS)
	@#$(LINK) -o $@ $^ $(LIBSERVER) $(UTILLIBS) $(LIBS)

.PHONY: static
static: $(OBJECTS) $(LIBSERVER)
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $^ $(LIBRARY) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(OBJSERVER): option.h server.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)" 

.PHONY: test
test:
	$(MAKE) -C $(testdir)
#	$(CUTTER) $(testdir)

.PHONY: install
install: all
	@test -z $(bindir) || mkdir -p $(bindir) || exit 1;
	$(INSTALL) $(PROGRAM) $(bindir)
	@test -z $(libdir) || mkdir -p $(libdir) || exit 1;
	$(INSTALL) $(LIBSERVER) $(libdir)
	$(INSTALL) $(SHAREDOBJ) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)
	$(STRIP) $(LIBSERVER)
	$(STRIP) $(SHAREDOBJ)

.PHONY: clean
clean:
	@$(RM) $(PROGRAM) $(OBJECTS) $(OBJSERVER) $(LIBSERVER) $(SHAREDOBJ)
	@$(MAKE) -C $(testdir) clean

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... test"
	@echo "... static"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


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
INCLUDES = -I$(srcdir) -I$(top_srcdir)/lib
CFLAGS = -g -Wall -O2 -fPIC -DNDEBUG -D_USE_SELECT
DFLAGS = -g -Wall -O2 -fPIC -D_DEBUG -DUNITTEST -D_USE_SELECT
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib
LIBS = -lpthread
UTILLIBS = -lcalcutil
CLIENTLIBS = -lcalcc
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a
LIBCLIENT = libcalcc.a
OBJCLIENT = client.o
OBJECTS = main.o option.o
SHAREDOBJ = libcalcc.so
PROGRAM = calcc
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ) $(PROGRAM)

$(LIBCLIENT): $(OBJCLIENT)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCLIENT)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(UTILLIBS) $(LIBS)

$(PROGRAM): $(OBJECTS)
	@$(RM) $@
	$(LINK) -o $@ $^ $(CLIENTLIBS) $(UTILLIBS) $(LIBS)
	@#$(LINK) -o $@ $^ $(LIBCLIENT) $(UTILLIBS) $(LIBS)

.PHONY: static
static: $(OBJECTS) $(LIBCLIENT)
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $^ $(LIBRARY) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(OBJCLIENT): option.h client.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test:
	$(MAKE) -C $(testdir)
	$(CUTTER) $(testdir)

.PHONY: install
install: all
	@test -z $(bindir) || mkdir -p $(bindir) || exit 1;
	$(INSTALL) $(PROGRAM) $(bindir)
	@test -z $(libdir) || mkdir -p $(libdir) || exit 1;
	$(INSTALL) $(LIBCLIENT) $(libdir)
	$(INSTALL) $(SHAREDOBJ) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)
	$(STRIP) $(LIBCLIENT)
	$(STRIP) $(SHAREDOBJ)

.PHONY: clean
clean:
	@$(RM) $(PROGRAM) $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ)
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


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
INCLUDES = -I$(srcdir)
CFLAGS = -g -Wall -O2 -fPIC -DHAVE_EXECINFO -DNDEBUG
DFLAGS = -g -Wall -O2 -fPIC -DHAVE_EXECINFO -DUNITTEST -D_DEBUG
LDFLAGS =
LIBS = -lpthread
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = libcalcutil.a
SHAREDOBJ = libcalcutil.so
OBJECTS = log.o \
          term.o \
          memfree.o \
          data.o \
          net.o \
          readline.o \
          fileio.o
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(LIBRARY) $(SHAREDOBJ)

$(LIBRARY): $(OBJECTS)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJECTS)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): data.h \
            memfree.h \
            log.h \
            net.h \
            readline.h \
            fileio.h \
            term.h \
            Makefile

.PHONY: debug
debug:
	$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test:
	$(MAKE) -C $(testdir)
	$(CUTTER) $(testdir)

.PHONY: install
install: all
	@test -z $(libdir) || mkdir -p $(libdir) || exit 1;
	$(INSTALL) $(LIBRARY) $(libdir)
	$(INSTALL) $(SHAREDOBJ) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(LIBRARY)
	$(STRIP) $(SHAREDOBJ)

.PHONY: clean
clean:
	@$(RM) $(OBJECTS) $(LIBRARY) $(SHAREDOBJ)
	@$(MAKE) -C $(testdir) clean

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... test"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


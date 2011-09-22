# Makefile
# $Id$

srcdir = .
top_srcdir = ..
prefix = /usr/local
libdir = $(prefix)/lib
bindir = $(prefix)/bin
libutil = ../../lib
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INSTALL = install -c
INCLUDES = -I$(libutil)/include
CFLAGS = -g -Wall -O2 -DNDEBUG
DFLAGS = -g -Wall -O2 -D_UT -D_DEBUG
LDFLAGS =
LIBS = -lpthread
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(CFLAGS)
LIBRARY = libcalcutil.a
SHAREDOBJ = libcalcutil.so
OBJECTS = data.o \
          util.o \
          net.o \
          readline.o \
          log.o
PYTHON = python
TESTOPT = -Wd -3 -E -tt

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(LIBRARY) $(SHAREDOBJ)

$(LIBRARY): $(OBJECTS)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJECTS)
	@$(RM) $@
	$(CC) -shared -Wl,-soname,$@ -o $@ $^

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): data.h \
            util.h \
            log.h \
            net.h \
            readline.h \
            version.h \
            Makefile

.PHONY: debug
debug:
	$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test:
	@for file in `ls $(testdir)`; do \
	    if test -n "`echo $$file | grep -E '*\.py$$'`"; then \
	        $(PYTHON) $(TESTOPT) "$(testdir)/$$file"; \
	    fi; \
	done

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
	$(RM) $(OBJECTS) $(LIBRARY) $(SHAREDOBJ)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... test"
	@echo "... install"
	@echo "... doc"
	@echo "... strip"


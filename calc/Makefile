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
CUTTER = /usr/bin/cutter -v v
INSTALL = install -c
INCLUDES = -I$(top_srcdir)/lib
CFLAGS = -g -Wall -O2 -DNDEBUG #-DHAVE_READLINE
DFLAGS = -g -Wall -O2 -DUNITTEST -D_DEBUG
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib
LIBS = -lm -lcalcutil -lpthread #-lreadline
CALCLIBS = -lcalcp
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a
LIBCALC = libcalcp.a
OBJCALC = error.o function.o calc.o
OBJECTS = main.o \
          option.o
SHAREDOBJ = libcalcp.so
PROGRAM = calcp
PYTHON = python
TESTOPT = -Wd -3 -E -tt

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCALC) $(LIBCALC) $(SHAREDOBJ) $(PROGRAM)

$(LIBCALC): $(OBJCALC)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCALC)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

$(PROGRAM): $(OBJECTS)
	@$(RM) $@
	$(LINK) -o $@ $(LIBS) $(CALCLIBS) $^

.PHONY: static
static: $(OBJECTS) $(OBJCALC) $(LIBCALC) $(SHAREDOBJ)
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $(LIBS) $(OBJECTS) $(LIBCALC) $(LIBRARY)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): option.h calc.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test:
	@#cd $(testdir) && $(MAKE)
	$(MAKE) -C $(testdir)
	$(CUTTER) $(testdir)
	@#for file in `ls $(testdir)`; do \
	    if test -n "`echo $$file | grep -E '*\.py$$'`"; then \
	        $(PYTHON) $(TESTOPT) "$(testdir)/$$file"; \
	    fi; \
	done

.PHONY: install
install: all
	@test -z $(bindir) || mkdir -p $(bindir) || exit 1;
	$(INSTALL) $(PROGRAM) $(bindir)
	@test -z $(libdir) || mkdir -p $(libdir) || exit 1;
	$(INSTALL) $(LIBCALC) $(libdir)
	$(INSTALL) $(SHAREDOBJ) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)
	$(STRIP) $(LIBCALC)
	$(STRIP) $(SHAREDOBJ)

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJECTS) $(OBJCALC) $(LIBCALC) $(SHAREDOBJ)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... static"
	@echo "... test"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


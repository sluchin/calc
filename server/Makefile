# Makefile
# $Id$

srcdir = .
top_srcdir = ..
prefix = /usr/local
libdir = $(prefix)/lib
bindir = $(prefix)/bin
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INSTALL = install -c
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/calc
CFLAGS = -g -Wall -O2 -DNDEBUG
DFLAGS = -g -Wall -O2 -D_UT -D_DEBUG
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib -L$(top_srcdir)/calc
LIBS = -lm -lpthread
SERVERLIBS = -lcalcd -lcalcp -lcalcutil
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a $(top_srcdir)/calc/libcalcp.a
LIBSERVER = libcalcd.a
OBJSERVER = server.o
OBJECTS = main.o option.o
SHAREDOBJ = libcalcd.so
PROGRAM = calcd
PYTHON = python
TESTOPT = -Wd -3 -E -tt

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJSERVER) $(LIBSERVER) $(SHAREDOBJ) $(PROGRAM)

$(LIBSERVER): $(OBJSERVER)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJSERVER)
	@$(RM) $@
	$(CC) -shared -Wl,-soname,$@ -o $@ $^

$(PROGRAM): $(OBJECTS)
	@$(RM) $(PROGRAM)
	$(LINK) -o $@ $(LIBS) $(SERVERLIBS) $^
	@#$(LINK) -o $@ $(LIBS) $^ $(LIBSERVER) $(LIBRARY)

.PHONY: static
static: $(OBJECTS) $(OBJSERVER) $(LIBSERVER) $(SHAREDOBJ)
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $(LIBS) $(OBJECTS) $(LIBSERVER) $(LIBRARY)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): option.h server.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)" 

.PHONY: test
test:
	@for file in `ls $(testdir)`; do \
	    if test -n "`echo $$file | grep -E '*\.py$$'`"; then \
	        $(PYTHON) $(TESTOPT) "$(testdir)/$$file"; \
	    fi; \
	done

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
	$(RM) $(PROGRAM) $(OBJECTS) $(OBJSERVER) $(LIBSERVER) $(SHAREDOBJ)

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


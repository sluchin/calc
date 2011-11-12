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
INCLUDES = -I$(top_srcdir)/lib
CFLAGS = -g -Wall -O2 -DNDEBUG -D_USE_SELECT #-DHAVE_READLINE
DFLAGS = -g -Wall -O2 -D_DEBUG -D_USE_SELECT #-DUNITTEST #-DHAVE_READLINE
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib
LIBS = -lreadline -lpthread
CLIENTLIBS = -lcalcc -lcalcutil
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a
LIBCLIENT = libcalcc.a
OBJCLIENT = client.o
OBJECTS = main.o option.o
SHAREDOBJ = libcalcc.so
PROGRAM = calcc
CUTTER = /usr/bin/cutter -v v
#PYTHON = python
#TESTOPT = -Wd -3 -E -tt

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ) $(PROGRAM)

$(LIBCLIENT): $(OBJCLIENT)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCLIENT)
	@$(RM) $@
	$(CC) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

$(PROGRAM): $(OBJECTS)
	@$(RM) $@
	$(LINK) -o $@ $(LIBS) $(CLIENTLIBS) $^
	@#$(LINK) -o $@ $(LIBS) $(LIBSCLIENT) $^

.PHONY: static
static: $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ)
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $(LIBS) $(OBJECTS) $(LIBCLIENT) $(LIBRARY)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): option.h client.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test:
	$(MAKE) -C $(testdir)
	$(CUTTER) $(testdir)
	#@for file in `ls $(testdir)`; do \
	    if test -n "`echo $$file | grep -E '*\.py$$'`"; then \
	        $(PYTHON) $(TESTOPT) "$(testdir)/$$file"; \
	    fi; \
	done

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
	$(RM) $(PROGRAM) $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ)
	$(MAKE) -C $(testdir) clean

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


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
INCLUDES = -I$(top_srcdir)/lib
CFLAGS = -g -Wall -O2 -DNDEBUG -D_USE_SELECT #-DHAVE_READLINE
DFLAGS = -g -Wall -O2 -D_USE_SELECT -D_UT -D_DEBUG #-DHAVE_READLINE
LDFLAGS = -L. -L$(top_srcdir)/lib
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
PYTHON = python
TESTOPT = -Wd -3 -E -tt

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ) $(PROGRAM)

$(LIBCLIENT): $(OBJCLIENT)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCLIENT)
	@$(RM) $@
	$(CC) -shared -Wl,-soname,$@ -o $@ $^

$(PROGRAM): $(OBJECTS)
	@$(RM) $@
	$(LINK) -o $@ $(LIBS) $^ $(LIBCLIENT) $(LIBRARY)
	@#$(LINK) -o $@ $(LIBS) $(LIBSCLIENT) $^

.PHONY: static
static: all
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $(LIBS) $(OBJECTS) $(LIBCLIENT) $(LIBRARY)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): option.h client.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: test
test: debug
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

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... test"
	@echo "... static"
	@echo "... install"
	@echo "... doc"
	@echo "... strip"


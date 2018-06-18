# Makefile

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
CFLAGS = -g -Wall -O2 -fPIC -DNDEBUG -DHAVE_READLINE
DFLAGS = -g -Wall -O2 -fPIC -DUNITTEST -D_DEBUG -DHAVE_READLINE
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib
LIBS = -lm -lpthread -lreadline
UTILLIBS = -lcalcutil
CALCLIBS = -lcalcp
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a
LIBCALC = libcalcp.a
OBJCALC = error.o func.o calc.o
OBJECTS = main.o \
          option.o
SHAREDOBJ = libcalcp.so
PROGRAM = calcp
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCALC) $(LIBCALC) $(SHAREDOBJ) $(PROGRAM)

$(LIBCALC): $(OBJCALC)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCALC)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(UTILLIBS) $(LIBS)

$(PROGRAM): $(OBJECTS)
	@$(RM) $@
	$(LINK) -o $@ $^ $(CALCLIBS) $(UTILLIBS) $(LIBS)
	@#$(LINK) -o $@ $^ $(LIBCALC) $(UTILLIBS) $(LIBS)

.PHONY: static
static: $(OBJECTS) $(LIBCALC)
	@$(RM) $(PROGRAM)
	$(LINK) -o $(PROGRAM) $^ $(LIBRARY) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(OBJCALC): option.h calc.h func.h error.h Makefile

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
	$(INSTALL) $(LIBCALC) $(libdir)
	$(INSTALL) $(SHAREDOBJ) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)
	$(STRIP) $(LIBCALC)
	$(STRIP) $(SHAREDOBJ)

.PHONY: clean
clean:
	@$(RM) $(PROGRAM) $(OBJECTS) $(OBJCALC) $(LIBCALC) $(SHAREDOBJ)
	@$(MAKE) -C $(testdir) clean

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


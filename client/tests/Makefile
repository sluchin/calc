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
CFLAGS = -g -Wall -O2 -fPIC -DUNITTEST
DFLAGS = -g -Wall -O2 -fPIC -DUNITTEST -D_DEBUG
LDFLAGS =  -L$(srcdir) -L$(pardir) -L$(libcalcdir)
LIBS = -lpthread
CLIENTLIBS = -lcalcc
CUTTERLIBS = -lcutter
UTILLIBS = -lcalcutil
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
CLIENTSOBJ = test_client.so
CLIENTOBJ = test_client.o
OBJECTS = thread_client.o
PROGRAM = thcalcc
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(CLIENTSOBJ) $(PROGRAM)

$(CLIENTSOBJ): $(CLIENTOBJ)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(CLIENTLIBS) $(CUTTERLIBS)

$(PROGRAM): $(OBJECTS)
	@$(RM) $@
	$(LINK) -o $@ $^ $(CLIENTLIBS) $(UTILLIBS) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(CLIENTOBJ): Makefile

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
	@$(RM) *.so *.o $(PROGRAM)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... test"
	@echo "... clean"
	@echo "... debug"
	@echo "... strip"
	@echo "... help"


# Makefile
# $Id$

srcdir = .
pardir = ..
top_srcdir = ../..
libcalcdir = $(top_srcdir)/lib
calcdir = $(top_srcdir)/calc
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES = -I$(srcdir) -I$(pardir) -I$(libcalcdir) -I$(calcdir) -I/usr/include/cutter
CFLAGS = -g -Wall -O2 -DUNITTEST
DFLAGS = -g -Wall -O2 -DUNITTEST -D_DEBUG
LDFLAGS =  -L$(srcdir) -L$(pardir) -L$(libcalcdir)
LIBS = -lcutter -lcalcd
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(LDFLAGS)
SERVERSOBJ = test_server.so
SERVEROBJ = test_server.o
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(SERVERSOBJ)

$(SERVERSOBJ): $(SERVEROBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

.c.o:
	$(COMPILE) -c $<

$(SERVEROBJ): Makefile

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
	@$(RM) *.so *.o

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... test"
	@echo "... clean"
	@echo "... debug"
	@echo "... strip"
	@echo "... help"


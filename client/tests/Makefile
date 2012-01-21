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
LIBS = -lcutter -lcalcc
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
CLIENTSOBJ = test_client.so
CLIENTOBJ = test_client.o
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(CLIENTSOBJ)

$(CLIENTSOBJ): $(CLIENTOBJ)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(CLIENTOBJ): Makefile

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


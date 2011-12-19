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
CFLAGS = -g -Wall -O2 -DUNITTEST
DFLAGS = -g -Wall -O2 -DUNITTEST -D_DEBUG
LDFLAGS =  -L$(srcdir) -L$(pardir) -L$(libcalcdir)
LIBS = -lcutter -lcalcutil -lpthread
TESTLIBS = -ltest_calc
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(LDFLAGS)
SERVEROBJ= test_server.o #$(pardir)/log.o
LOGSOBJ = test_log.so
LOGOBJ = test_log.o
NETSOBJ = test_net.so
NETOBJ = test_net.o
#ERRORSOBJ = test_error.so
#CALCOBJ = test_calc.o
#FUNCOBJ = test_function.o
#ERROROBJ = test_error.o
#COMMONOBJ = test_common.o
#PROGRAM = server
CUTTER = /usr/bin/cutter -v v

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(LOGSOBJ) $(NETSOBJ)

$(LOGSOBJ): $(LOGOBJ) $(SERVEROBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

$(NETSOBJ): $(NETOBJ)
	@$(RM) $@
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

#
#$(FUNCSOBJ): $(FUNCOBJ) $(COMMONOBJ)
#	@$(RM) $@
#	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^
#
#$(ERRORSOBJ): $(ERROROBJ) $(COMMONOBJ)
#	@$(RM) $@
#	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $(LIBS) $^

#$(PROGRAM): $(OBJECTS)
#	@$(RM) $@
#	$(LINK) -o $@ $(LIBS) $^

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): Makefile

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
	$(RM) *.so *.o

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... test"
	@echo "... clean"
	@echo "... debug"
	@echo "... strip"
	@echo "... help"


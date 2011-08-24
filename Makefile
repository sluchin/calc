# Makefile
# $Id$

srcdir = .
top_srcdir = .
CC = gcc
RM = rm -rf
AR = ar rcsv
STRIP = strip
INCLUDES =
CFLAGS += -g -Wall -O2
LDFLAGS =
LIBS = -lm
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(CC) $(CFLAGS)
LIBRARY =
PROGRAM = $(top_srcdir)/calc/calcp \
          $(top_srcdir)/client/calcc \
          $(top_srcdir)/server/calcd

.PHONY: all doc debug strip clean

all:
	cd $(top_srcdir)/lib && $(MAKE) $@
	cd $(top_srcdir)/calc && $(MAKE) $@
	cd $(top_srcdir)/server && $(MAKE) $@
	cd $(top_srcdir)/client && $(MAKE) $@

doc:
	doxygen Doxyfile

debug:
	cd $(top_srcdir)/lib && $(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 
	cd $(top_srcdir)/calc && $(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 
	cd $(top_srcdir)/server && $(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 
	cd $(top_srcdir)/client && $(MAKE) CFLAGS="-g -Wall -O2 -D_DEBUG" 

strip:
	$(STRIP) $(PROGRAM)

clean:
	cd $(top_srcdir)/lib && $(MAKE) clean
	cd $(top_srcdir)/calc && $(MAKE) clean
	cd $(top_srcdir)/server && $(MAKE) clean
	cd $(top_srcdir)/client && $(MAKE) clean


# Makefile
# $Id$
srcdir = .
top_srcdir = ..
prefix = /usr/local
libdir = $(prefix)/lib
bindir = $(prefix)/bin
CC = gcc
RM = rm -rf
AR = ar rcs
STRIP = strip
INSTALL = install -c
INCLUDES = -I$(srcdir)
CFLAGS = -g -Wall -O2 -fPIC -DHAVE_EXECINFO -DNDEBUG
DFLAGS = -g -Wall -O2 -fPIC -DHAVE_EXECINFO -DUNITTEST -D_DEBUG
LDFLAGS =
LIBS = -lpthread
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = libcalcutil.a
SHAREDOBJ = libcalcutil.so
OBJECTS = log.o \
          term.o \
          memfree.o \
          data.o \
          net.o \
          readline.o \
          fileio.o

# DYNAMIC=1 (make DYNAMIC=1) のとき, 動的ライブラリ (.so) を作る.
# 既定は, 静的ライブラリ (.a) を作る.
ifdef DYNAMIC
LIBTARGET = $(SHAREDOBJ)
else
LIBTARGET = $(LIBRARY)
endif

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(LIBTARGET)

$(LIBRARY): $(OBJECTS)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJECTS)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS): data.h \
            memfree.h \
            log.h \
            net.h \
            readline.h \
            fileio.h \
            term.h \
            Makefile

.PHONY: debug
debug:
	$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: install
install: all
	@test -z $(libdir) || mkdir -p $(libdir) || exit 1;
	$(INSTALL) $(LIBTARGET) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(LIBTARGET)

.PHONY: clean
clean:
	@$(RM) $(OBJECTS) $(LIBRARY) $(SHAREDOBJ)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


# Makefile

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

# DYNAMIC=1 (make DYNAMIC=1) のとき, 動的ライブラリ (.so) を作って, リンクする.
# 既定は, 静的ライブラリ (.a) を作って, リンクする.
ifdef DYNAMIC
LIBTARGET = $(SHAREDOBJ)
PROGLIBS = $(CALCLIBS) $(UTILLIBS)
else
LIBTARGET = $(LIBCALC)
PROGLIBS = $(LIBCALC) $(LIBRARY)
endif

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCALC) $(LIBTARGET) $(PROGRAM)

$(LIBCALC): $(OBJCALC)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCALC)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(UTILLIBS) $(LIBS)

$(PROGRAM): $(OBJECTS) $(LIBTARGET)
	@$(RM) $@
	$(LINK) -o $@ $(OBJECTS) $(PROGLIBS) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(OBJCALC): option.h calc.h func.h error.h Makefile

.PHONY: debug
debug:
	@$(MAKE) CFLAGS="$(DFLAGS)"

.PHONY: install
install: all
	@test -z $(bindir) || mkdir -p $(bindir) || exit 1;
	$(INSTALL) $(PROGRAM) $(bindir)
	@test -z $(libdir) || mkdir -p $(libdir) || exit 1;
	$(INSTALL) $(LIBTARGET) $(libdir)

.PHONY: strip
strip:
	$(STRIP) $(PROGRAM)
	$(STRIP) $(LIBTARGET)

.PHONY: clean
clean:
	@$(RM) $(PROGRAM) $(OBJECTS) $(OBJCALC) $(LIBCALC) $(SHAREDOBJ)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


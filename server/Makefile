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
include $(top_srcdir)/warnings.mk
INCLUDES = -I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/calc
CFLAGS = -g $(WARNFLAGS) -O2 -fPIC -DNDEBUG
DFLAGS = -g $(WARNFLAGS) -O2 -fPIC -DUNITTEST -D_DEBUG
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib -L$(top_srcdir)/calc
LIBS = -lm -lpthread
UTILLIBS = -lcalcp -lcalcutil
SERVERLIBS = -lcalcd
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a $(top_srcdir)/calc/libcalcp.a
LIBSERVER = libcalcd.a
OBJSERVER = server.o
OBJECTS = main.o option.o
SHAREDOBJ = libcalcd.so
PROGRAM = calcd

# DYNAMIC=1 (make DYNAMIC=1) のとき, 動的ライブラリ (.so) を作って, リンクする.
# 既定は, 静的ライブラリ (.a) を作って, リンクする.
ifdef DYNAMIC
LIBTARGET = $(SHAREDOBJ)
PROGLIBS = $(SERVERLIBS) $(UTILLIBS)
else
LIBTARGET = $(LIBSERVER)
PROGLIBS = $(LIBSERVER) $(top_srcdir)/calc/libcalcp.a $(top_srcdir)/lib/libcalcutil.a
endif

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJSERVER) $(LIBTARGET) $(PROGRAM)

$(LIBSERVER): $(OBJSERVER)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJSERVER)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(UTILLIBS) $(LIBS)

$(PROGRAM): $(OBJECTS) $(LIBTARGET)
	@$(RM) $@
	$(LINK) -o $@ $(OBJECTS) $(PROGLIBS) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(OBJSERVER): option.h server.h Makefile

# 静的解析 (gcc -fanalyzer. gcc 10 以降). 通常のビルドとは分けてあり, 解析に時間がかかる.
# リリース用 (CFLAGS) とデバッグ用 (DFLAGS) の両方の設定で, 全ソースを解析する.
# 指摘 (警告) があれば, 失敗する.
# 最適化をすると, 不要なコードが消えて, 指摘できなくなることがあるので, -O0 で解析する.
.PHONY: analyze
analyze:
	@for fl in "$(CFLAGS)" "$(DFLAGS)"; do \
	    for f in $(wildcard *.c); do \
	        echo "analyze: $$f"; \
	        $(CC) $(INCLUDES) $$fl -O0 -fanalyzer -Werror -c -o /dev/null $$f || exit 1; \
	    done; \
	done

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
	@$(RM) $(PROGRAM) $(OBJECTS) $(OBJSERVER) $(LIBSERVER) $(SHAREDOBJ)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


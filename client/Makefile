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
INCLUDES = -I$(srcdir) -I$(top_srcdir)/lib
CFLAGS = -g $(WARNFLAGS) -O2 -fPIC -DNDEBUG -D_USE_SELECT
DFLAGS = -g $(WARNFLAGS) -O2 -fPIC -D_DEBUG -DUNITTEST -D_USE_SELECT
LDFLAGS = -L$(srcdir) -L$(top_srcdir)/lib
LIBS = -lpthread
UTILLIBS = -lcalcutil
CLIENTLIBS = -lcalcc
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(LDFLAGS)
LIBRARY = $(top_srcdir)/lib/libcalcutil.a
LIBCLIENT = libcalcc.a
OBJCLIENT = client.o
OBJECTS = main.o option.o
SHAREDOBJ = libcalcc.so
PROGRAM = calcc

# DYNAMIC=1 (make DYNAMIC=1) のとき, 動的ライブラリ (.so) を作って, リンクする.
# 既定は, 静的ライブラリ (.a) を作って, リンクする.
ifdef DYNAMIC
LIBTARGET = $(SHAREDOBJ)
PROGLIBS = $(CLIENTLIBS) $(UTILLIBS)
else
LIBTARGET = $(LIBCLIENT)
PROGLIBS = $(LIBCLIENT) $(LIBRARY)
endif

.SUFFIXES: .c .o

.PHONY: all
all: $(OBJECTS) $(OBJCLIENT) $(LIBTARGET) $(PROGRAM)

$(LIBCLIENT): $(OBJCLIENT)
	@$(RM) $@
	$(AR) $@ $^

$(SHAREDOBJ): $(OBJCLIENT)
	@$(RM) $@
	$(LINK) -shared -Wl,-soname,$@ -o $@ $^ $(UTILLIBS) $(LIBS)

$(PROGRAM): $(OBJECTS) $(LIBTARGET)
	@$(RM) $@
	$(LINK) -o $@ $(OBJECTS) $(PROGLIBS) $(LIBS)

.c.o:
	$(COMPILE) -c $<

$(OBJECTS) $(OBJCLIENT): option.h client.h Makefile

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
	@$(RM) $(PROGRAM) $(OBJECTS) $(OBJCLIENT) $(LIBCLIENT) $(SHAREDOBJ)

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... install"
	@echo "... strip"
	@echo "... help"


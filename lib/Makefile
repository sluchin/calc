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
INCLUDES = -I$(srcdir)
CFLAGS = -g $(WARNFLAGS) -O2 -fPIC -DHAVE_EXECINFO -DNDEBUG
DFLAGS = -g $(WARNFLAGS) -O2 -fPIC -DHAVE_EXECINFO -DUNITTEST -D_DEBUG
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
          fileio.o \
          sig.o

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
            sig.h \
            term.h \
            Makefile

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


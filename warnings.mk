# warnings.mk
#
# $(top_srcdir)/warnings.txt の警告オプションのうち, $(CC) が受け付けるものだけを,
# WARNFLAGS にする. まとめて受け付けられればそれを使い, 受け付けられなければ, 1 つずつ確認する.
# (include する前に, top_srcdir と CC を定義しておくこと)
WARNLIST := $(shell sed 's/\#.*//' $(top_srcdir)/warnings.txt)
WARNFLAGS := $(shell flags="$(WARNLIST)"; \
    if $(CC) $$flags -fsyntax-only -x c /dev/null >/dev/null 2>&1; then \
        echo $$flags; \
    else \
        for f in $$flags; do \
            $(CC) $$f -fsyntax-only -x c /dev/null >/dev/null 2>&1 && echo $$f; \
        done; \
    fi)

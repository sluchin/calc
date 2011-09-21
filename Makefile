# Makefile
# $Id$

srcdir = .
top_srcdir = .

.PHONY: all
all:
	@failcom='exit 1'; \
	(cd $(top_srcdir)/lib && $(MAKE)) || eval $$failcom; \
	(cd $(top_srcdir)/calc && $(MAKE)) || eval $$failcom; \
	(cd $(top_srcdir)/server && $(MAKE)) || eval $$failcom; \
	(cd $(top_srcdir)/client && $(MAKE)) || eval $$failcom; \
	echo ""; \
	echo "*******************************"; \
	echo "* Success!! Congratulations!! *"; \
	echo "* make all finished.          *"; \
	echo "*******************************"; \
	echo "";

.PHONY: static
static:
	cd $(top_srcdir)/lib && $(MAKE)
	cd $(top_srcdir)/calc && $(MAKE) static
	cd $(top_srcdir)/server && $(MAKE) static
	cd $(top_srcdir)/client && $(MAKE) static

.PHONY: debug
debug:
	@failcom='exit 1'; \
	(cd $(top_srcdir)/lib && $(MAKE) debug) || eval $$failcom; \
	(cd $(top_srcdir)/calc && $(MAKE) debug) || eval $$failcom; \
	(cd $(top_srcdir)/server && $(MAKE) debug) || eval $$failcom; \
	(cd $(top_srcdir)/client && $(MAKE) debug) || eval $$failcom; \
	echo ""; \
	echo "*******************************"; \
	echo "* Success!! Congratulations!! *"; \
	echo "* make debug finished.        *"; \
	echo "*******************************"; \
	echo "";

.PHONY: test
test:
	cd $(top_srcdir)/lib && $(MAKE) test
	cd $(top_srcdir)/calc && $(MAKE) test
	cd $(top_srcdir)/server && $(MAKE) test
	cd $(top_srcdir)/client && $(MAKE) test

.PHONY: install
install:
	cd $(top_srcdir)/lib && $(MAKE) install
	cd $(top_srcdir)/calc && $(MAKE) install
	cd $(top_srcdir)/server && $(MAKE) install
	cd $(top_srcdir)/client && $(MAKE) install

.PHONY: strip
strip:
	cd $(top_srcdir)/lib && $(MAKE) strip
	cd $(top_srcdir)/calc && $(MAKE) strip
	cd $(top_srcdir)/server && $(MAKE) strip
	cd $(top_srcdir)/client && $(MAKE) strip

.PHONY: clean
clean:
	cd $(top_srcdir)/lib && $(MAKE) clean
	cd $(top_srcdir)/calc && $(MAKE) clean
	cd $(top_srcdir)/server && $(MAKE) clean
	cd $(top_srcdir)/client && $(MAKE) clean

.PHONY: doc
doc:
	doxygen Doxyfile

.PHONY: help
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... debug"
	@echo "... static"
	@echo "... test"
	@echo "... doc"
	@echo "... strip"


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

.PHONY: cmake-build
cmake-build:
	@mkdir -p build
	@cd build && cmake .. && $(MAKE)

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
	@cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug
	@cd build-test && ctest --verbose

.PHONY: coverage
coverage:
	@cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=ON
	@cmake --build build-coverage
	@cmake --build build-coverage --target coverage

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
	@rm -rf doc build build-test build-coverage
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
	@echo "... test (cmake, ctest)"
	@echo "... coverage (cmake, gcovr)"
	@echo "... doc"
	@echo "... strip"


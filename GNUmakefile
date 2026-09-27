# Makefile
# $Id$

srcdir = .
top_srcdir = .
prefix = /usr/local
libdir = $(prefix)/lib

# 既定は, 静的ライブラリ (.a). make DYNAMIC=1 で, 動的ライブラリ (.so).
# (GNU make の -D は, 変数の定義ではないので, DYNAMIC=1 と書く. サブディレクトリの
# Makefile にも, コマンドラインの変数は, 引き継がれる)

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
ifdef DYNAMIC
	@echo ""
	@echo "動的ライブラリを $(libdir) にインストールしました."
	@echo "実行するには, ライブラリの検索パスに $(libdir) が含まれることを確認して,"
	@echo "ldconfig を実行してください: sudo ldconfig"
	@echo "($(libdir) が含まれない場合は, /etc/ld.so.conf.d/ にファイルを作って追加する)"
else
	@echo ""
	@echo "静的ライブラリを使ったので, 実行ファイルは, 単独で動きます. (ldconfig は不要)"
	@echo "動的ライブラリを使うには, make clean してから, make DYNAMIC=1 install"
endif

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
	@echo "... install"
	@echo "DYNAMIC=1 を付けると, 動的ライブラリ (.so) を作る (既定は, 静的ライブラリ (.a))"
	@echo "... test (cmake, ctest)"
	@echo "... coverage (cmake, gcovr)"
	@echo "... doc"
	@echo "... strip"


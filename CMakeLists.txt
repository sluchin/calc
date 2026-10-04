# CMakeLists.txt
#
# 使い方:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   (Debug も可)
#   cmake --build build
#   cd build && make test    (単体テストをビルドしてから, 実行する. make ではビルドしない)
#
# Debug では、Makefile の debug ターゲットと同じく -DUNITTEST -D_DEBUG を付ける.

cmake_minimum_required(VERSION 3.10)
project(calc C)

# ビルドタイプが指定されなければ, Release にする (-DCMAKE_BUILD_TYPE=Debug で明示できる).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "ビルドタイプ" FORCE)
endif()

# 最適化は, Makefile (GNU make) に合わせて, Debug/Release とも -g -O2 にする.
set(CMAKE_C_FLAGS_DEBUG "-g -O2")
set(CMAKE_C_FLAGS_RELEASE "-g -O2")
# 静的ライブラリも, 動的ライブラリにリンクできるように, PIC (-fPIC) を付ける.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# カバレッジ (gcov). 正確に測るため, 最適化とインライン展開はしない.
option(ENABLE_COVERAGE "カバレッジを計測する (gcov)" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -O0 -fno-inline)
  add_link_options(--coverage)
endif()
# 警告オプションは, warnings.txt に一覧がある (Makefile と共通). 警告が出たら修正する.
include(cmake/warnings.cmake)
# 単体テスト (tests/) をビルドして, make test (ctest) で実行できるようにする.
# 単体テストは, UNITTEST で公開される内部関数を使うので, 有効なときは全体に
# -DUNITTEST を付ける. 配布用のビルドでは -DBUILD_TESTS=OFF にする.
option(BUILD_TESTS "単体テストをビルドする (UNITTEST を有効にする)" ON)
# 単体テストは, 標準ライブラリの関数 (vsnprintf など) を, モックに置き換える.
# _FORTIFY_SOURCE が有効だと, __vsnprintf_chk などに置き換わって, モックにできない.
if(BUILD_TESTS)
  add_compile_options(-U_FORTIFY_SOURCE)
endif()
add_compile_definitions(
  $<$<OR:$<CONFIG:Debug>,$<BOOL:${BUILD_TESTS}>>:UNITTEST>
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

# 依存ライブラリ: pthread, libm (数学関数), GNU readline (calcp の入力補完に必要).
find_package(Threads REQUIRED)
find_library(MATH_LIBRARY m)
find_library(READLINE_LIBRARY readline)
if(NOT READLINE_LIBRARY)
  message(FATAL_ERROR "GNU readline が見つかりません (libreadline-dev)")
endif()

# install 先の標準的なディレクトリ (CMAKE_INSTALL_BINDIR/LIBDIR など) を決める.
include(GNUInstallDirs)

# ライブラリの種類: 既定は, 静的ライブラリ (.a). -DDYNAMIC=ON で, 動的ライブラリ (.so).
# 静的と動的は, 両方は作らない (実行ファイルは, 選んだ方にリンクする).
option(DYNAMIC "動的ライブラリ (.so) を作って, リンクする (OFF は, 静的ライブラリ (.a))" OFF)
if(DYNAMIC)
  set(LIB_TYPE SHARED)
else()
  set(LIB_TYPE STATIC)
endif()

# lib: libcalcutil
set(UTIL_SOURCES
  lib/log.c lib/term.c lib/memfree.c lib/data.c
  lib/net.c lib/readline.c lib/fileio.c lib/sig.c)
add_library(calcutil_obj OBJECT ${UTIL_SOURCES})
target_include_directories(calcutil_obj PUBLIC lib)
target_compile_definitions(calcutil_obj PUBLIC HAVE_EXECINFO)
add_library(calcutil_lib ${LIB_TYPE} $<TARGET_OBJECTS:calcutil_obj>)
set_target_properties(calcutil_lib PROPERTIES OUTPUT_NAME calcutil)
target_include_directories(calcutil_lib PUBLIC lib)
target_compile_definitions(calcutil_lib PUBLIC HAVE_EXECINFO)
target_link_libraries(calcutil_lib PUBLIC Threads::Threads)

# calc: libcalcp, calcp
set(CALC_SOURCES calc/error.c calc/func.c calc/calc.c)
add_library(calcp_obj OBJECT ${CALC_SOURCES})
target_include_directories(calcp_obj PUBLIC calc lib)
target_compile_definitions(calcp_obj PUBLIC HAVE_READLINE)
add_library(calcp_lib ${LIB_TYPE} $<TARGET_OBJECTS:calcp_obj>)
set_target_properties(calcp_lib PROPERTIES OUTPUT_NAME calcp)
target_include_directories(calcp_lib PUBLIC calc lib)
target_compile_definitions(calcp_lib PUBLIC HAVE_READLINE)
target_link_libraries(calcp_lib PUBLIC calcutil_lib ${MATH_LIBRARY}
                      ${READLINE_LIBRARY} Threads::Threads)
add_executable(calcp calc/main.c calc/option.c)
target_link_libraries(calcp PRIVATE calcp_lib)

# server: libcalcd, calcd
add_library(calcd_obj OBJECT server/server.c)
target_include_directories(calcd_obj PUBLIC server calc lib)
add_library(calcd_lib ${LIB_TYPE} $<TARGET_OBJECTS:calcd_obj>)
set_target_properties(calcd_lib PROPERTIES OUTPUT_NAME calcd)
target_include_directories(calcd_lib PUBLIC server calc lib)
target_link_libraries(calcd_lib PUBLIC calcp_lib calcutil_lib)
add_executable(calcd server/main.c server/option.c)
target_link_libraries(calcd PRIVATE calcd_lib)

# client: libcalcc, calcc
add_library(calcc_obj OBJECT client/client.c)
target_include_directories(calcc_obj PUBLIC client lib)
target_compile_definitions(calcc_obj PUBLIC _USE_SELECT)
add_library(calcc_lib ${LIB_TYPE} $<TARGET_OBJECTS:calcc_obj>)
set_target_properties(calcc_lib PROPERTIES OUTPUT_NAME calcc)
target_include_directories(calcc_lib PUBLIC client lib)
target_compile_definitions(calcc_lib PUBLIC _USE_SELECT)
target_link_libraries(calcc_lib PUBLIC calcutil_lib)
add_executable(calcc client/main.c client/option.c)
target_link_libraries(calcc PRIVATE calcc_lib)

# make analyze: 静的解析 (gcc -fanalyzer. gcc 10 以降). 通常のビルドには含まれない.
# 全ソースを, ビルドタイプの設定で解析する. 指摘 (警告) があれば, 失敗する.
set(ANALYZE_TARGETS "")
foreach(comp IN ITEMS calcutil calcp calcd calcc)
  get_target_property(analyze_sources ${comp}_obj SOURCES)
  if(comp STREQUAL "calcp")
    list(APPEND analyze_sources calc/main.c calc/option.c)
  elseif(comp STREQUAL "calcd")
    list(APPEND analyze_sources server/main.c server/option.c)
  elseif(comp STREQUAL "calcc")
    list(APPEND analyze_sources client/main.c client/option.c)
  endif()
  add_library(${comp}_analyze OBJECT EXCLUDE_FROM_ALL ${analyze_sources})
  target_include_directories(${comp}_analyze PRIVATE
    $<TARGET_PROPERTY:${comp}_obj,INTERFACE_INCLUDE_DIRECTORIES>)
  target_compile_definitions(${comp}_analyze PRIVATE
    $<TARGET_PROPERTY:${comp}_obj,INTERFACE_COMPILE_DEFINITIONS>)
  # 最適化をすると, 不要なコードが消えて, 指摘できなくなることがあるので, -O0 で解析する
  target_compile_options(${comp}_analyze PRIVATE -O0 -fanalyzer -Werror)
  list(APPEND ANALYZE_TARGETS ${comp}_analyze)
endforeach()
add_custom_target(analyze DEPENDS ${ANALYZE_TARGETS})

# install: 実行ファイルは bin へ, ライブラリは, 静的 (ARCHIVE) か動的 (LIBRARY) かに
# 応じて lib へインストールする (/usr/local が既定. Makefile の make install に相当).
install(TARGETS calcp calcd calcc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS calcutil_lib calcp_lib calcd_lib calcc_lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# make install のあとに, ライブラリの種類に応じたメッセージを表示する
if(DYNAMIC)
  install(CODE "
    message(STATUS \"\")
    message(STATUS \"動的ライブラリを \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR} にインストールしました.\")
    message(STATUS \"実行するには, ライブラリの検索パスにそのディレクトリが含まれることを確認して,\")
    message(STATUS \"ldconfig を実行してください: sudo ldconfig\")
    message(STATUS \"(含まれない場合は, /etc/ld.so.conf.d/ にファイルを作って追加する)\")
  ")
else()
  install(CODE "
    message(STATUS \"\")
    message(STATUS \"静的ライブラリを使ったので, 実行ファイルは, 単独で動きます. (ldconfig は不要)\")
    message(STATUS \"動的ライブラリを使うには, cmake -DDYNAMIC=ON で構成し直してください.\")
  ")
endif()

# 単体テスト (greatest + FFF): tests/ 配下
if(BUILD_TESTS)
  # make test (ctest) の出力: ビルド (build_tests) の詳細と, 各テストの出力を表示する.
  # 失敗したときの出力も含まれる. (CMake 3.17 以降)
  set(CMAKE_CTEST_ARGUMENTS --verbose)
  enable_testing()
  add_subdirectory(tests)

  get_property(unit_test_targets GLOBAL PROPERTY UNIT_TEST_TARGETS)

  # make coverage: テストを実行して, カバレッジレポートを作る.
  # gcovr が必要.
  find_program(GCOVR_COMMAND gcovr)
  if(ENABLE_COVERAGE AND GCOVR_COMMAND)
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
      # 実行ファイル (calcp calcd calcc) の main.c は, テストでは実行されない.
      # テストは main.c を別の名前 (xxx_main) でリンクするので, 同じ行が別の関数と
      # 見なされて, 集計が 0 になる. そのオブジェクトの計測情報は, 消しておく.
      COMMAND find ${CMAKE_BINARY_DIR}/CMakeFiles -path "*/calc[pdc].dir/*"
              -name "*.gcno" -delete
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${GCOVR_COMMAND} -r ${CMAKE_SOURCE_DIR} --object-directory ${CMAKE_BINARY_DIR}
              --filter ${CMAKE_SOURCE_DIR}/lib/
              --filter ${CMAKE_SOURCE_DIR}/calc/
              --filter ${CMAKE_SOURCE_DIR}/server/
              --filter ${CMAKE_SOURCE_DIR}/client/
              --html-details ${CMAKE_BINARY_DIR}/coverage/index.html
              --print-summary --txt
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "カバレッジレポート: ${CMAKE_BINARY_DIR}/coverage/index.html"
      VERBATIM)
    add_dependencies(coverage ${unit_test_targets})
  elseif(GCOVR_COMMAND)
    # カバレッジ用の構成 (-DENABLE_COVERAGE=ON) でなくても, make coverage で使えるように,
    # 別のビルドディレクトリ (coverage-build) に, カバレッジ用にビルドして実行する.
    # Release にするのは, Debug の dbglog() (デバッグ用のログ) を, 集計に含めないため.
    set(cov_dir ${CMAKE_BINARY_DIR}/coverage-build)
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${cov_dir}
              -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON
      COMMAND ${CMAKE_COMMAND} --build ${cov_dir}
      COMMAND ${CMAKE_COMMAND} --build ${cov_dir} --target coverage
      COMMENT "カバレッジレポート: ${cov_dir}/coverage/index.html"
      VERBATIM)
  else()
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E echo
              "coverage は使えません: gcovr をインストールしてください (sudo apt install gcovr)"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM)
  endif()
endif()

# make doc: Doxygen でドキュメントを生成する. 出力は, <ビルドディレクトリ>/docs/html/
# (doxygen が必要. グラフを出力するには, graphviz の dot も必要)
# make clean で, ターゲットの出力 (実行ファイルやライブラリ) 以外の, 次のものも, ディレクトリごと
# 削除する (CMake 3.15 以降): ドキュメント, カバレッジのレポート, ctest の出力 (Testing),
# 単体テストの実行ファイルのディレクトリ (実行ファイルは, clean が消して, 空になる).
# tests/ 自体は, ビルドに必要な生成ファイル (Makefile など) があるので, 削除しない.
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
             ${CMAKE_BINARY_DIR}/docs
             ${CMAKE_BINARY_DIR}/coverage
             ${CMAKE_BINARY_DIR}/coverage-build
             ${CMAKE_BINARY_DIR}/Testing
             ${CMAKE_BINARY_DIR}/tests/lib
             ${CMAKE_BINARY_DIR}/tests/calcp
             ${CMAKE_BINARY_DIR}/tests/calcd
             ${CMAKE_BINARY_DIR}/tests/calcc)

# doxygen (と, グラフ描画用の dot) が見つかれば, doc ターゲットを, ドキュメントを生成するように
# 構成する. 見つからなければ, 実行時にエラーメッセージを表示するだけのターゲットにする.
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
  if(TARGET Doxygen::dot)
    set(doxygen_have_dot YES)
  else()
    set(doxygen_have_dot NO)
  endif()
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND}
            -DDOXYGEN=${DOXYGEN_EXECUTABLE}
            -DDOXYFILE=${CMAKE_SOURCE_DIR}/Doxyfile
            -DSRC=${CMAKE_SOURCE_DIR}
            -DOUT=${CMAKE_BINARY_DIR}/docs
            -DHAVE_DOT=${doxygen_have_dot}
            -P ${CMAKE_SOURCE_DIR}/cmake/doxygen.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Doxygen: ${CMAKE_BINARY_DIR}/docs/html/index.html"
    VERBATIM)
else()
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND} -E echo
            "doc は使えません: doxygen をインストールしてください (sudo apt install doxygen graphviz)"
    COMMAND ${CMAKE_COMMAND} -E false
    VERBATIM)
endif()

# カバレッジ (ENABLE_COVERAGE) の, 計測ファイル (.gcno と .gcda) も, make clean で削除する.
# .o と同じ場所に, <ソース>.gcno と <ソース>.gcda ができるので, ソースから, パスを作る.
# (ソースがターゲットのディレクトリの外にあると, パスの ../ は __/ になる)
function(add_coverage_clean_files dir)
  get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
  foreach(t ${targets})
    get_target_property(type ${t} TYPE)
    if(type STREQUAL "UTILITY" OR type STREQUAL "INTERFACE_LIBRARY")
      continue()
    endif()
    get_target_property(srcs ${t} SOURCES)
    get_target_property(srcdir ${t} SOURCE_DIR)
    get_target_property(bindir ${t} BINARY_DIR)
    foreach(src ${srcs})
      if(NOT src MATCHES "\\.c$")
        continue() # $<TARGET_OBJECTS:...> などは, 対象外
      endif()
      if(NOT IS_ABSOLUTE ${src})
        set(src ${srcdir}/${src})
      endif()
      file(RELATIVE_PATH rel ${srcdir} ${src})
      string(REPLACE "../" "__/" rel "${rel}")
      set_property(TARGET ${t} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
                   ${bindir}/CMakeFiles/${t}.dir/${rel}.gcno
                   ${bindir}/CMakeFiles/${t}.dir/${rel}.gcda)
    endforeach()
  endforeach()
endfunction()
if(ENABLE_COVERAGE)
  add_coverage_clean_files(${CMAKE_SOURCE_DIR})
  if(BUILD_TESTS)
    add_coverage_clean_files(${CMAKE_SOURCE_DIR}/tests)
  endif()
endif()

# --- 整形 (clang-format) ---
# 設定は, .clang-format にある. 使い方:
#   cmake --build build --target format          ソースを整形する (ファイルを書き換える)
#   cmake --build build --target format-check    整形が必要なファイルがあれば, 失敗する (書き換えない)
# clang-format が, PATH にないときは, -DCLANG_FORMAT=/path/to/clang-format で指定する.
find_program(CLANG_FORMAT NAMES clang-format)
file(GLOB_RECURSE FORMAT_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/calc/*.c ${CMAKE_SOURCE_DIR}/calc/*.h
     ${CMAKE_SOURCE_DIR}/client/*.c ${CMAKE_SOURCE_DIR}/client/*.h
     ${CMAKE_SOURCE_DIR}/server/*.c ${CMAKE_SOURCE_DIR}/server/*.h
     ${CMAKE_SOURCE_DIR}/lib/*.c ${CMAKE_SOURCE_DIR}/lib/*.h
     ${CMAKE_SOURCE_DIR}/tests/*.c ${CMAKE_SOURCE_DIR}/tests/*.h)
# 外部のソース (tests/third_party) と, ビルドの出力は, 整形しない.
list(FILTER FORMAT_SOURCES EXCLUDE REGEX "/third_party/|/build[^/]*/")
if(CLANG_FORMAT)
  add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i --style=file ${FORMAT_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "clang-format でソースを整形します"
    VERBATIM)
  add_custom_target(format-check
    COMMAND ${CLANG_FORMAT} --dry-run --Werror --style=file ${FORMAT_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "clang-format で整形の必要を確認します"
    VERBATIM)
else()
  foreach(t format format-check)
    add_custom_target(${t}
      COMMAND ${CMAKE_COMMAND} -E echo
              "${t} は使えません: clang-format をインストールしてください (sudo apt install clang-format)"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM)
  endforeach()
endif()

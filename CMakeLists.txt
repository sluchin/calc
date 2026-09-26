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

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "ビルドタイプ" FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG "-g -O2")
set(CMAKE_C_FLAGS_RELEASE "-g -O2")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# カバレッジ (gcov). 正確に測るため, 最適化とインライン展開はしない.
option(ENABLE_COVERAGE "カバレッジを計測する (gcov)" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -O0 -fno-inline)
  add_link_options(--coverage)
endif()
add_compile_options(-Wall)
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

find_package(Threads REQUIRED)
find_library(MATH_LIBRARY m)
find_library(READLINE_LIBRARY readline)
if(NOT READLINE_LIBRARY)
  message(FATAL_ERROR "GNU readline が見つかりません (libreadline-dev)")
endif()

include(GNUInstallDirs)

# lib: libcalcutil
set(UTIL_SOURCES
  lib/log.c lib/term.c lib/memfree.c lib/data.c
  lib/net.c lib/readline.c lib/fileio.c)
add_library(calcutil_obj OBJECT ${UTIL_SOURCES})
target_include_directories(calcutil_obj PUBLIC lib)
target_compile_definitions(calcutil_obj PUBLIC HAVE_EXECINFO)
add_library(calcutil_static STATIC $<TARGET_OBJECTS:calcutil_obj>)
add_library(calcutil_shared SHARED $<TARGET_OBJECTS:calcutil_obj>)
foreach(t calcutil_static calcutil_shared)
  set_target_properties(${t} PROPERTIES OUTPUT_NAME calcutil)
  target_include_directories(${t} PUBLIC lib)
  target_compile_definitions(${t} PUBLIC HAVE_EXECINFO)
  target_link_libraries(${t} PUBLIC Threads::Threads)
endforeach()

# calc: libcalcp, calcp
set(CALC_SOURCES calc/error.c calc/func.c calc/calc.c)
add_library(calcp_obj OBJECT ${CALC_SOURCES})
target_include_directories(calcp_obj PUBLIC calc lib)
target_compile_definitions(calcp_obj PUBLIC HAVE_READLINE)
add_library(calcp_static STATIC $<TARGET_OBJECTS:calcp_obj>)
add_library(calcp_shared SHARED $<TARGET_OBJECTS:calcp_obj>)
foreach(t calcp_static calcp_shared)
  set_target_properties(${t} PROPERTIES OUTPUT_NAME calcp)
  target_include_directories(${t} PUBLIC calc lib)
  target_compile_definitions(${t} PUBLIC HAVE_READLINE)
  target_link_libraries(${t} PUBLIC calcutil_shared ${MATH_LIBRARY}
                        ${READLINE_LIBRARY} Threads::Threads)
endforeach()
add_executable(calcp calc/main.c calc/option.c)
target_link_libraries(calcp PRIVATE calcp_shared)

# server: libcalcd, calcd
add_library(calcd_obj OBJECT server/server.c)
target_include_directories(calcd_obj PUBLIC server calc lib)
add_library(calcd_static STATIC $<TARGET_OBJECTS:calcd_obj>)
add_library(calcd_shared SHARED $<TARGET_OBJECTS:calcd_obj>)
foreach(t calcd_static calcd_shared)
  set_target_properties(${t} PROPERTIES OUTPUT_NAME calcd)
  target_include_directories(${t} PUBLIC server calc lib)
  target_link_libraries(${t} PUBLIC calcp_shared calcutil_shared)
endforeach()
add_executable(calcd server/main.c server/option.c)
target_link_libraries(calcd PRIVATE calcd_shared)

# client: libcalcc, calcc
add_library(calcc_obj OBJECT client/client.c)
target_include_directories(calcc_obj PUBLIC client lib)
target_compile_definitions(calcc_obj PUBLIC _USE_SELECT)
add_library(calcc_static STATIC $<TARGET_OBJECTS:calcc_obj>)
add_library(calcc_shared SHARED $<TARGET_OBJECTS:calcc_obj>)
foreach(t calcc_static calcc_shared)
  set_target_properties(${t} PROPERTIES OUTPUT_NAME calcc)
  target_include_directories(${t} PUBLIC client lib)
  target_compile_definitions(${t} PUBLIC _USE_SELECT)
  target_link_libraries(${t} PUBLIC calcutil_shared)
endforeach()
add_executable(calcc client/main.c client/option.c)
target_link_libraries(calcc PRIVATE calcc_shared)

# install
install(TARGETS calcp calcd calcc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS calcutil_static calcutil_shared calcp_static calcp_shared
                calcd_static calcd_shared calcc_static calcc_shared
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

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

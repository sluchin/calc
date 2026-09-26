# CMakeLists.txt
#
# 使い方:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   (Debug も可)
#   cmake --build build
#   ctest --test-dir build --output-on-failure       (cutter がある場合)
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
add_compile_options(-Wall)
add_compile_definitions(
  $<$<CONFIG:Debug>:UNITTEST>
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

# 単体テスト (cutter)
find_program(CUTTER_COMMAND cutter)
find_path(CUTTER_INCLUDE_DIR cutter.h PATH_SUFFIXES cutter)
find_library(CUTTER_LIBRARY cutter)
if(CUTTER_COMMAND AND CUTTER_INCLUDE_DIR AND CUTTER_LIBRARY)
  enable_testing()
  # cutter は, テストを共有ライブラリとして読み込んで実行する.
  function(add_cutter_test dir name)
    add_library(${name} MODULE ${dir}/tests/${name}.c ${ARGN})
    set_target_properties(${name} PROPERTIES PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${dir})
    target_include_directories(${name} PRIVATE
      ${dir}/tests ${dir} ${CUTTER_INCLUDE_DIR})
    target_compile_definitions(${name} PRIVATE HAVE_EXECINFO UNITTEST)
    target_link_libraries(${name} PRIVATE ${CUTTER_LIBRARY}
      calcutil_shared calcp_shared calcd_shared calcc_shared)
  endfunction()
  foreach(n log net data readline memfree timer fileio term)
    if(EXISTS ${CMAKE_SOURCE_DIR}/lib/tests/test_${n}.c)
      add_cutter_test(lib test_${n})
    endif()
  endforeach()
  foreach(n calc func error)
    add_cutter_test(calc test_${n} calc/tests/test_common.c)
  endforeach()
  add_cutter_test(server test_server)
  add_cutter_test(client test_client)
  foreach(d lib calc server client)
    add_test(NAME ${d}
      COMMAND ${CUTTER_COMMAND} -v v ${CMAKE_BINARY_DIR}/tests/${d})
  endforeach()
else()
  message(STATUS "cutter が見つからないため, 単体テストは無効")
endif()

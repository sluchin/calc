# cmake/warnings.cmake
#
# warnings.txt (Makefile と共通) の警告オプションのうち, コンパイラが受け付けるものを,
# 全体に付ける.
include(CheckCCompilerFlag)

file(STRINGS ${CMAKE_SOURCE_DIR}/warnings.txt WARNING_LINES)
set(WARNING_FLAGS "")
foreach(line IN LISTS WARNING_LINES)
  string(REGEX REPLACE "#.*" "" flag "${line}")
  string(STRIP "${flag}" flag)
  if(flag STREQUAL "")
    continue()
  endif()
  string(MAKE_C_IDENTIFIER "HAVE_WARNING${flag}" flag_var)
  check_c_compiler_flag("${flag}" ${flag_var})
  if(${flag_var})
    list(APPEND WARNING_FLAGS ${flag})
  endif()
endforeach()
add_compile_options(${WARNING_FLAGS})

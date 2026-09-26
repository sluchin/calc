# tests/CMakeLists.txt
#
# 単体テスト. テストランナーは greatest, モックは FFF (どちらも third_party/).
# ソースと同じ構成で, lib/ calcp/ calcd/ calcc/ に置く.
# テスト 1 ファイルが, 1 つの実行ファイルと 1 つの ctest テストになる.

# add_unit_test(<ディレクトリ> <名前>
#               LIBS <ライブラリ...> INCLUDES <ディレクトリ...>
#               [SOURCES <追加ソース...>] [RENAME_MAIN <名前>])
# 実行ファイル名は <ディレクトリ>_<名前>, 出力ファイル名は <名前> にする.
# option.h のように, ディレクトリ間で同名のヘッダがあるので, INCLUDES には
# テスト対象のディレクトリだけを指定する.
function(add_unit_test dir name)
  cmake_parse_arguments(T "" "RENAME_MAIN" "LIBS;INCLUDES;SOURCES" ${ARGN})
  set(target ${dir}_${name})
  add_executable(${target} ${dir}/${name}.c ${T_SOURCES})
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
    ${T_INCLUDES})
  if(T_RENAME_MAIN) # SOURCES の先頭の main() は, 名前を変える (テストの main は変えない)
    list(GET T_SOURCES 0 main_source)
    set_source_files_properties(${main_source} PROPERTIES
      COMPILE_DEFINITIONS main=${T_RENAME_MAIN})
  endif()
  target_link_libraries(${target} PRIVATE ${T_LIBS} m)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir})
  add_test(NAME ${dir}/${name} COMMAND ${target})
  set_tests_properties(${dir}/${name} PROPERTIES TIMEOUT 60)
endfunction()

set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(CALC_DIR ${CMAKE_SOURCE_DIR}/calc)
set(SERVER_DIR ${CMAKE_SOURCE_DIR}/server)
set(CLIENT_DIR ${CMAKE_SOURCE_DIR}/client)

foreach(n data fileio log memfree net readline term timer)
  add_unit_test(lib test_${n} LIBS calcutil_shared INCLUDES ${LIB_DIR})
endforeach()

foreach(n calc error func)
  add_unit_test(calcp test_${n}
    LIBS calcp_shared calcutil_shared INCLUDES ${CALC_DIR} ${LIB_DIR}
    SOURCES calcp/helper.c)
endforeach()
add_unit_test(calcd test_server
  LIBS calcd_shared calcp_shared calcutil_shared
  INCLUDES ${SERVER_DIR} ${CALC_DIR} ${LIB_DIR})
add_unit_test(calcc test_client
  LIBS calcc_shared calcutil_shared INCLUDES ${CLIENT_DIR} ${LIB_DIR})

# option.c, main.c は, テストの実行ファイルにソースごとリンクして, 依存する関数
# を FFF でモックにする. main() は, 名前を変えて (xxx_main), 子プロセスから呼ぶ.
add_unit_test(calcp test_option
  LIBS calcp_shared calcutil_shared INCLUDES ${CALC_DIR} ${LIB_DIR}
  SOURCES ${CALC_DIR}/option.c)
add_unit_test(calcd test_option
  LIBS calcd_shared calcp_shared calcutil_shared
  INCLUDES ${SERVER_DIR} ${CALC_DIR} ${LIB_DIR}
  SOURCES ${SERVER_DIR}/option.c)
add_unit_test(calcc test_option
  LIBS calcc_shared calcutil_shared INCLUDES ${CLIENT_DIR} ${LIB_DIR}
  SOURCES ${CLIENT_DIR}/option.c)

add_unit_test(calcp test_main
  LIBS calcp_shared calcutil_shared INCLUDES ${CALC_DIR} ${LIB_DIR}
  SOURCES ${CALC_DIR}/main.c RENAME_MAIN calcp_main)
add_unit_test(calcd test_main
  LIBS calcd_shared calcp_shared calcutil_shared
  INCLUDES ${SERVER_DIR} ${CALC_DIR} ${LIB_DIR}
  SOURCES ${SERVER_DIR}/main.c RENAME_MAIN calcd_main)
add_unit_test(calcc test_main
  LIBS calcc_shared calcutil_shared INCLUDES ${CLIENT_DIR} ${LIB_DIR}
  SOURCES ${CLIENT_DIR}/main.c RENAME_MAIN calcc_main)

# 負荷確認用のクライアント (テストではない)
add_executable(thcalcc calcc/thread_client.c)
target_include_directories(thcalcc PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party
  ${CLIENT_DIR} ${LIB_DIR})
target_link_libraries(thcalcc PRIVATE calcc_shared calcutil_shared
                      Threads::Threads)

# tests/CMakeLists.txt
#
# 単体テスト. テストランナーは greatest, モックは FFF (どちらも third_party/).
# ソースと同じ構成で, lib/ calcp/ calcd/ calcc/ に置く.
# テスト 1 ファイルが, 1 つの実行ファイルと 1 つの ctest テストになる.

# forkpty() (tests/test_process.h) は, 古い glibc では libutil にある
find_library(UTIL_LIBRARY util)

# テストの引数 (argv) は, 文字列リテラルで初期化する (char *argv[] = { "prog", ... }).
# その const は, テスト対象の関数 (char *argv[]) に合わせて外せないので, 警告を出さない.
add_compile_options(-Wno-write-strings)
# モック (FFF の FAKE_VALUE_FUNC など) は, libc の関数 (close, send など) を, 同じ型で再宣言する.
add_compile_options(-Wno-redundant-decls)

# add_unit_test(<ディレクトリ> <名前>
#               LIBS <ライブラリ...> INCLUDES <ディレクトリ...>
#               [SOURCES <追加ソース...>] [RENAME_MAIN <名前>])
# 実行ファイル名は <ディレクトリ>_<名前>, 出力ファイル名は <名前> にする.
# option.h のように, ディレクトリ間で同名のヘッダがあるので, INCLUDES には
# テスト対象のディレクトリだけを指定する.
function(add_unit_test dir name)
  cmake_parse_arguments(T "" "RENAME_MAIN" "LIBS;INCLUDES;SOURCES" ${ARGN})
  set(target ${dir}_${name})
  # make (引数なし) ではビルドしない. make test (ctest) のときに, ビルドする
  add_executable(${target} EXCLUDE_FROM_ALL ${dir}/${name}.c ${T_SOURCES})
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
    ${T_INCLUDES})
  if(T_RENAME_MAIN) # SOURCES の先頭の main() は, 名前を変える (テストの main は変えない)
    list(GET T_SOURCES 0 main_source)
    # 名前を変えた main() は, ヘッダに宣言がなく, exit() で終わる関数になるので, その警告は出さない
    set_source_files_properties(${main_source} PROPERTIES
      COMPILE_DEFINITIONS main=${T_RENAME_MAIN}
      COMPILE_OPTIONS "-Wno-missing-prototypes;-Wno-missing-declarations;-Wno-missing-noreturn")
  endif()
  # dlsym() (test_helper.h の TEST_PASSTHROUGH) のため, _GNU_SOURCE と dl が必要
  target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
  target_link_libraries(${target} PRIVATE ${T_LIBS} m ${CMAKE_DL_LIBS}
                        ${UTIL_LIBRARY})
  set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir})
  # make clean は, 出力先のディレクトリも削除する (トップレベルの CMakeLists.txt) ので,
  # リンクの前に作る (リンカは, ディレクトリを作らない)
  add_custom_command(TARGET ${target} PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${dir}
    VERBATIM)
  set_property(GLOBAL APPEND PROPERTY UNIT_TEST_TARGETS ${target})
  add_test(NAME ${dir}/${name} COMMAND ${target})
  set_tests_properties(${dir}/${name} PROPERTIES TIMEOUT 60
                       FIXTURES_REQUIRED tests_built)
endfunction()

# 単体テストが使うライブラリ.
# テストは, ライブラリの中の関数を, テストの実行ファイルに同名の関数を定義して, 置き換える
# (モック). 静的ライブラリでは, 関数が, 同じオブジェクトの他の関数と一緒にリンクされて,
# 多重定義になるので, 共有ライブラリにする. (製品のライブラリの種類 (DYNAMIC) とは関係
# なく, 同じオブジェクトから, テスト専用に作る. インストールしない)
add_library(calcutil_testlib SHARED EXCLUDE_FROM_ALL
            $<TARGET_OBJECTS:calcutil_obj>)
set_target_properties(calcutil_testlib PROPERTIES OUTPUT_NAME calcutil_test)
target_include_directories(calcutil_testlib PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_compile_definitions(calcutil_testlib PUBLIC HAVE_EXECINFO)
target_link_libraries(calcutil_testlib PUBLIC Threads::Threads)

add_library(calcp_testlib SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:calcp_obj>)
set_target_properties(calcp_testlib PROPERTIES OUTPUT_NAME calcp_test)
target_include_directories(calcp_testlib PUBLIC
  ${CMAKE_SOURCE_DIR}/calc ${CMAKE_SOURCE_DIR}/lib)
target_compile_definitions(calcp_testlib PUBLIC HAVE_READLINE)
target_link_libraries(calcp_testlib PUBLIC calcutil_testlib ${MATH_LIBRARY}
                      ${READLINE_LIBRARY} Threads::Threads)

add_library(calcd_testlib SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:calcd_obj>)
set_target_properties(calcd_testlib PROPERTIES OUTPUT_NAME calcd_test)
target_include_directories(calcd_testlib PUBLIC
  ${CMAKE_SOURCE_DIR}/server ${CMAKE_SOURCE_DIR}/calc ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(calcd_testlib PUBLIC calcp_testlib calcutil_testlib)

add_library(calcc_testlib SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:calcc_obj>)
set_target_properties(calcc_testlib PROPERTIES OUTPUT_NAME calcc_test)
target_include_directories(calcc_testlib PUBLIC
  ${CMAKE_SOURCE_DIR}/client ${CMAKE_SOURCE_DIR}/lib)
target_compile_definitions(calcc_testlib PUBLIC _USE_SELECT)
target_link_libraries(calcc_testlib PUBLIC calcutil_testlib)

set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(CALC_DIR ${CMAKE_SOURCE_DIR}/calc)
set(SERVER_DIR ${CMAKE_SOURCE_DIR}/server)
set(CLIENT_DIR ${CMAKE_SOURCE_DIR}/client)

foreach(n data fileio log memfree net readline sig term timer)
  add_unit_test(lib test_${n} LIBS calcutil_testlib INCLUDES ${LIB_DIR})
endforeach()

foreach(n calc error func)
  add_unit_test(calcp test_${n}
    LIBS calcp_testlib calcutil_testlib INCLUDES ${CALC_DIR} ${LIB_DIR}
    SOURCES calcp/helper.c)
endforeach()
add_unit_test(calcd test_server
  LIBS calcd_testlib calcp_testlib calcutil_testlib
  INCLUDES ${SERVER_DIR} ${CALC_DIR} ${LIB_DIR})
add_unit_test(calcc test_client
  LIBS calcc_testlib calcutil_testlib INCLUDES ${CLIENT_DIR} ${LIB_DIR})

# calcd/test_server と calcc/test_client は, 同じポート番号 (12345) を使うので,
# ctest -j で並列に実行しても, 同時には実行されないようにする.
set_tests_properties(calcd/test_server calcc/test_client
                     PROPERTIES RESOURCE_LOCK port12345)

# option.c, main.c は, テストの実行ファイルにソースごとリンクして, 依存する関数
# を FFF でモックにする. main() は, 名前を変えて (xxx_main), 子プロセスから呼ぶ.
add_unit_test(calcp test_option
  LIBS calcp_testlib calcutil_testlib INCLUDES ${CALC_DIR} ${LIB_DIR}
  SOURCES ${CALC_DIR}/option.c)
add_unit_test(calcd test_option
  LIBS calcd_testlib calcp_testlib calcutil_testlib
  INCLUDES ${SERVER_DIR} ${CALC_DIR} ${LIB_DIR}
  SOURCES ${SERVER_DIR}/option.c)
add_unit_test(calcc test_option
  LIBS calcc_testlib calcutil_testlib INCLUDES ${CLIENT_DIR} ${LIB_DIR}
  SOURCES ${CLIENT_DIR}/option.c)

add_unit_test(calcp test_main
  LIBS calcp_testlib calcutil_testlib INCLUDES ${CALC_DIR} ${LIB_DIR}
  SOURCES ${CALC_DIR}/main.c RENAME_MAIN calcp_main)
add_unit_test(calcd test_main
  LIBS calcd_testlib calcp_testlib calcutil_testlib
  INCLUDES ${SERVER_DIR} ${CALC_DIR} ${LIB_DIR}
  SOURCES ${SERVER_DIR}/main.c RENAME_MAIN calcd_main)
add_unit_test(calcc test_main
  LIBS calcc_testlib calcutil_testlib INCLUDES ${CLIENT_DIR} ${LIB_DIR}
  SOURCES ${CLIENT_DIR}/main.c RENAME_MAIN calcc_main)

# 負荷確認用のクライアント (テストではない)
add_executable(thcalcc EXCLUDE_FROM_ALL calcc/thread_client.c)
target_include_directories(thcalcc PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party
  ${CLIENT_DIR} ${LIB_DIR})
target_link_libraries(thcalcc PRIVATE calcc_lib calcutil_lib
                      Threads::Threads)

# make test は, テストの実行ファイルをビルドしてから, テストを実行する.
# CMake 組み込みの test ターゲットは, ビルドに依存させられないので, ビルドを,
# 全てのテストが必要とする ctest のテスト (fixture) にする.
get_property(unit_test_targets GLOBAL PROPERTY UNIT_TEST_TARGETS)
add_custom_target(unit_tests DEPENDS thcalcc)
add_dependencies(unit_tests ${unit_test_targets})
add_test(NAME build_tests
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                 --config $<CONFIG> --target unit_tests)
set_tests_properties(build_tests PROPERTIES FIXTURES_SETUP tests_built)

# cmake/doxygen.cmake
#
# Doxygen でドキュメントを生成する (make doc から呼ばれる).
# Doxyfile の後ろに, ビルドディレクトリ用の設定を追加して, 上書きする.
#
# 引数: DOXYGEN (doxygen のパス), DOXYFILE, SRC (ソースの最上位), OUT (出力先),
#       HAVE_DOT (YES または NO)

file(READ "${DOXYFILE}" config)
string(APPEND config "
# make doc が追加した設定
OUTPUT_DIRECTORY = ${OUT}
HAVE_DOT         = ${HAVE_DOT}
")
file(MAKE_DIRECTORY "${OUT}")
file(WRITE "${OUT}/Doxyfile.generated" "${config}")

# Doxyfile の INPUT は, ソースの最上位からの相対パスなので, そこで実行する
execute_process(
  COMMAND "${DOXYGEN}" "${OUT}/Doxyfile.generated"
  WORKING_DIRECTORY "${SRC}"
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "doxygen が失敗しました (${result})")
endif()
message(STATUS "ドキュメント: ${OUT}/html/index.html")

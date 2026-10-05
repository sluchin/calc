/**
 * @file tests/lib/test_memfree.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-22 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h> /* malloc */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "memfree.h"

/* プロトタイプ */
TEST test_memfree(void);

/**
 * set_memfree() 関数テスト
 */
TEST
test_memfree(void)
{
    char *mem[] = {NULL, NULL, NULL}; /* ポインタ値 */
    enum { MEM1, MEM2, MEM3, MAX };   /* 配列要素 */

    int i;
    for (i = 0; i < MAX; i++) {
        mem[i] = (char *)malloc(5U * sizeof(char));
        if (mem[i] == NULL) {
            TEST_FAIL("malloc");
        }
    }
    memfree(&mem[MEM1], &mem[MEM2], &mem[MEM3], NULL);
    TEST_ASSERT_NULL(mem[MEM1]);
    TEST_ASSERT_NULL(mem[MEM2]);
    TEST_ASSERT_NULL(mem[MEM3]);

    /* 第二引数がNULLの場合 */
    mem[MEM1] = (char *)malloc(5U * sizeof(char));
    if (mem[MEM1] == NULL) {
        TEST_FAIL("malloc");
    }
    mem[MEM3] = (char *)malloc(5U * sizeof(char));
    if (mem[MEM3] == NULL) {
        TEST_FAIL("malloc");
    }
    memfree(&mem[MEM1], &mem[MEM2], &mem[MEM3], NULL);
    TEST_ASSERT_NULL(mem[MEM1]);
    TEST_ASSERT_NULL(mem[MEM3]);
    PASS();
}

/* greatest の定義 (main() を含む, 実行ファイルごとに 1 か所) */
GREATEST_MAIN_DEFS();

/**
 * テストの実行
 *
 * @param[in] argc 引数の数
 * @param[in] argv 引数 (greatest のオプション. -t の後にテスト名を指定すると, 1 つのテストだけ実行できる)
 * @return 全てのテストが成功なら EXIT_SUCCESS, 失敗があれば EXIT_FAILURE
 */
int
main(int argc, char **argv)
{
    /* greatest の初期化 (オプションの解析. 標準出力のバッファリングは行わない) */
    TEST_MAIN_BEGIN();
    /* テストの実行 */
    RUN_TEST(test_memfree);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}

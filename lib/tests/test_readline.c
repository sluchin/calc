/**
 * @file lib/tests/test_readline.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-20 higashi 新規作成
 * @version \Id
 *
 * Copyright (C) 2011 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <unistd.h> /* pipe fork */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "readline.h"
#include "test_common.h"

/* 端末の入力可能なバイト数(4096)より大きいサイズに設定 */
#define BUF_SIZE 4100 /**< バッファサイズ */

/* プロトタイプ */
/** readline() 関数テスト */
void test_readline(void);

/* 内部関数 */
/** データ送受信 */
static char *write_data(char *data, size_t length);

/**
 * set_client_data() 関数テスト
 *
 * @return なし
 */
void
test_readline(void)
{
    char test_data[BUF_SIZE] = {0}; /* テストデータ */
    char nolf_data[] = "test";      /* 改行なし文字列 */
    char *result = NULL;            /* 結果文字列 */

    /* 正常系 */
    (void)memset(test_data, 0x31, sizeof(test_data) - 2);
    test_data[sizeof(test_data) - 1] = '\n';

    result = write_data(test_data, sizeof(test_data));

    if (test_data[strlen(test_data) - 1] == '\n')
        test_data[strlen(test_data) - 1] = '\0';
    cut_assert_equal_string(test_data, result);

    /* 異常系 */
    /* 改行ない場合 */
    /* fgetsの引数に渡すバッファサイズより小さい場合, NULLを返す */
    result = write_data(nolf_data, sizeof(nolf_data));
    dbglog("result=%s", result);
    cut_assert_equal_string("", result);

    /* ファイルポインタがNULLの場合 */
    result = (char *)_readline((FILE *)NULL);
    cut_assert_null(result);
}

/**
 * データ送受信
 *
 * @param[in] data テストデータ
 * @param[in] length バイト数
 * @return 結果文字列
 */
static char *
write_data(char *data, size_t length)
{
    FILE *fp = NULL;                    /* ファイルポインタ */
    uchar *result = NULL;               /* 結果文字列 */
    int pfd1[MAX_PIPE], pfd2[MAX_PIPE]; /* パイプ */
    int retval = 0;                     /* 戻り値 */
    pid_t pid = 0;                      /* プロセスID */
    ssize_t len = 0;                    /* readn, writen 戻り値 */
    static char readbuf[BUF_SIZE];      /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    retval = pipe(pfd1);
    if (retval < 0) {
        cut_error("pipe=%d", retval);
        return NULL;
    }

    retval = pipe(pfd2);
    if (retval < 0) {
        cut_error("pipe=%d", retval);
        return NULL;
    }

    pid = fork();
    if (pid < 0) {
        cut_error("fork=%d", pid);
        return NULL;
    }

    if (pid == 0) { /* 子プロセス */
        close_fd(&pfd1[PIPE_W], NULL);
        fp = fdopen(pfd1[PIPE_R], "r");
        if (!fp) {
            cut_error("fdopen=%p", fp);
            close_fd(&pfd1[PIPE_R], &pfd2[PIPE_R], &pfd2[PIPE_W], NULL);
            exit(EXIT_FAILURE);
        }
        /* 受信待ち */
        result = _readline(fp);
        if (!result) {
            close_fd(&pfd1[PIPE_R], &pfd2[PIPE_R], &pfd2[PIPE_W], NULL);
            exit(EXIT_FAILURE);
        }
        dbglog("length=%zu, result=%s", strlen((char *)result), result);
        close_fd(&pfd1[PIPE_R], &pfd2[PIPE_R], NULL);
        /* 送信 */
        len = writen(pfd2[PIPE_W], result, strlen((char *)result) + 1);
        if (len < 0) {
            cut_error("len=%zd", len);
            close_fd(&pfd2[PIPE_W], NULL);
            if (result) free(result);
            exit(EXIT_FAILURE);
        }
        dbglog("write: len=%zd", len);
        close_fd(&pfd2[PIPE_W], NULL);
        if (result) free(result);
        exit(EXIT_SUCCESS);
    } else { /* 親プロセス */
        close_fd(&pfd1[PIPE_R], NULL);
        /* 送信 */
        len = writen(pfd1[PIPE_W], data, length);
        if (len < 0) {
            cut_error("len=%zd", len);
            close_fd(&pfd1[PIPE_W], &pfd2[PIPE_W], &pfd2[PIPE_R], NULL);
            return NULL;
        }
        close_fd(&pfd1[PIPE_W], &pfd2[PIPE_W], NULL);
        /* 受信 */
        len = readn(pfd2[PIPE_R], readbuf, sizeof(readbuf));
        if (len < 0)
            cut_error("len=%zd", len);
        dbglog("read: len=%zd", len);
        close_fd(&pfd2[PIPE_R], NULL);
    }
    return readbuf;
}


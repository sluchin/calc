/**
 * @file lib/tests/test_readline.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-20 higashi 新規作成
 * @version \$Id$
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

#include <unistd.h>   /* pipe fork */
#include <sys/wait.h> /* wait waitpid */
#include <errno.h>    /* errno */
#include <signal.h>   /* signal */
#include <cutter.h>   /* cutter library */

#include "def.h"
#include "log.h"
#include "memfree.h"
#include "fileio.h"
#include "readline.h"

/* 端末の入力可能なバイト数(4096)より大きいサイズに設定 */
#define BUF_SIZE 1100 /**< バッファサイズ */

/* プロトタイプ */
/** readline() 関数テスト */
void test_readline(void);

/* 内部変数 */
static int pfd[] = { -1, -1 };   /* パイプ1 */
static char test_data[BUF_SIZE]; /* テストデータ */
static uchar *result = NULL;     /* 結果文字列 */

/* 内部関数 */
/** readline() 実行 */
static uchar *exec_readline(char *data, size_t length);
/** シグナル設定 */
static void set_sig_handler(void);

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_startup(void)
{
    set_sig_handler();
}

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_setup(void)
{
    (void)memset(test_data, 0x31, sizeof(test_data));
    test_data[sizeof(test_data) - 1] = '\0';
    test_data[sizeof(test_data) - 2] = '\n';
}

/**
 * 終了処理
 *
 * @return なし
 */
void
cut_teardown(void)
{
    close_fd(&pfd[PIPE_R], &pfd[PIPE_W], NULL);
    memfree((void **)&result, NULL);
}

/**
 * readline() 関数テスト
 *
 * @return なし
 */
void
test_readline(void)
{
    char nolf_data[] = "test"; /* 改行なし文字列 */

    /* 正常系 */
    result = exec_readline(test_data, sizeof(test_data));

    /* 改行削除 */
    if (test_data[strlen(test_data) - 1] == '\n')
        test_data[strlen(test_data) - 1] = '\0';

    cut_assert_equal_string(test_data, (char *)result);

    memfree((void **)&result, NULL);

    /* 異常系 */
    /* 改行ない場合 */
    result = exec_readline(nolf_data, sizeof(nolf_data));
    dbglog("result=%s", result);
    cut_assert_null((char *)result);

    /* ファイルポインタがNULLの場合 */
    result = _readline((FILE *)NULL);
    cut_assert_null((char *)result);
}

/**
 * readline() 実行
 *
 * @param[in] data テストデータ
 * @param[in] length バイト数
 * @return 結果文字列
 */
static uchar *
exec_readline(char *data, size_t length)
{
    FILE *fp = NULL; /* ファイルポインタ */
    int retval = 0;  /* 戻り値 */
    pid_t cpid = 0;  /* プロセスID */
    pid_t w = 0;     /* wait戻り値 */
    int status = 0;  /* ステイタス */
    ssize_t len = 0; /* writen 戻り値 */

    retval = pipe(pfd);
    if (retval < 0) {
        cut_error("pipe=%d", retval);
        return NULL;
    }

    fp = fdopen(pfd[PIPE_R], "r");
    if (!fp) {
        cut_error("fdopen=%p", fp);
        return NULL;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return NULL;
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        close_fd(&pfd[PIPE_R], NULL);

        /* 送信 */
        len = writen(pfd[PIPE_W], data, length);
        if (len < 0) {
            outlog("writen");
            close_fd(&pfd[PIPE_W], NULL);
            exit(EXIT_FAILURE);
        }
        close_fd(&pfd[PIPE_W], NULL);
        exit(EXIT_SUCCESS);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        close_fd(&pfd[PIPE_W], NULL);

        /* テスト関数の実行 */
        /* 受信待ち */
        result = _readline(fp);
        dbglog("result=%s", result);

        close_fd(&pfd[PIPE_R], NULL);
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            cut_notify("wait: status=%d(%d)", status, errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status)) {
            cut_notify("child error");
            return NULL;
        }
    }
    return result;
}

/**
 * シグナル設定
 *
 * @return なし
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
    if (signal(SIGINT, SIG_IGN) < 0)
        cut_notify("SIGINT");
    if (signal(SIGTERM, SIG_IGN) < 0)
        cut_notify("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) < 0)
        cut_notify("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) < 0)
        cut_notify("SIGHUP");
    if (signal(SIGALRM, SIG_IGN) < 0)
        cut_notify("SIGALRM");
}


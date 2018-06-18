/**
 * @file lib/tests/test_term.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-01-06 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
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

#include <unistd.h>  /* STDIN_FILENO */
#include <termios.h> /* termios */
#include <errno.h>   /* errno */
#include <cutter.h>  /* cutter library */

#include "def.h"
#include "log.h"
#include "fileio.h"
#include "term.h"

#define BUF_SIZE 2048 /**< バッファサイズ */

/* プロトタイプ */
/** sys_print_termattr() 関数テスト */
void test_sys_print_termattr(void);
/** get_termattr() 関数テスト */
void test_get_termattr(void);
/** mode_type_flag() 関数テスト */
void test_mode_type_flag(void);

/* 内部変数 */
static testterm term; /**< 関数構造体 */
static int fd = -1;   /**< ファイルディスクリプタ */

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_startup(void)
{
    (void)memset(&term, 0, sizeof(testterm));
    test_init_term(&term);
}

/**
 * 終了処理
 *
 * @return なし
 */
void
cut_teardown(void)
{
    if (fd != -1) {
        if (close(fd) < 0)
            cut_notify("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }
}

/**
 * print_termattr() 関数テスト
 *
 * @return なし
 */
void
test_sys_print_termattr(void)
{
    int rlen = 0;                /* 戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "programname\\[[0-9]+\\]: filename\\[15\\]: function: " \
        "tcgetattr\\(.*\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd(%d)", errno);
        return;
    }

    sys_print_termattr(LOG_INFO, LOG_PID | LOG_PERROR, "programname",
                       "filename", 15, "function", STDIN_FILENO);

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0) {
        cut_fail("read: fd=%d(%d)", fd, errno);
        return;
    }

    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));
}

/**
 * get_termattr() 関数テスト
 *
 * @return なし
 */
void
test_get_termattr(void)
{
    char *ptr = NULL;    /* テスト関数戻り値 */
    struct termios mode; /* termios構造体 */

    (void)memset(&mode, 0, sizeof(struct termios));
    ptr = term.get_termattr(STDIN_FILENO, &mode);

    cut_assert_match("tcgetattr(.*)", ptr);

    if (ptr)
        free(ptr);
    ptr = NULL;
}

/**
 * mode_type_flag() 関数テスト
 *
 * @return なし
 */
void
test_mode_type_flag(void)
{
    tcflag_t *flag = NULL; /* テスト関数戻り値 */
    struct termios mode;   /* termios構造体 */
    int retval = 0;        /* 戻り値 */

    (void)memset(&mode, 0, sizeof(struct termios));
    retval = tcgetattr(STDIN_FILENO, &mode);
    if (retval < 0) {
        cut_fail("tcgetattr: mode=%p(%d)", &mode, errno);
        return;
    }

    /* 正常系 */
    flag = term.mode_type_flag(control, &mode);
    cut_assert_not_null(flag);
    flag = term.mode_type_flag(input, &mode);
    cut_assert_not_null(flag);
    flag = term.mode_type_flag(output, &mode);
    cut_assert_not_null(flag);
    flag = term.mode_type_flag(local, &mode);
    cut_assert_not_null(flag);
    /* 異常系 */
    flag = term.mode_type_flag((enum mode_type)4, &mode);
    cut_assert_null(flag);
}


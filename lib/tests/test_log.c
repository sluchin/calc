/**
 * @file lib/tests/test_log.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-11-19 higashi 新規作成
 * @version \$Id
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

#include <stdio.h>  /* snprintf tmpnam */
#include <unistd.h> /* STDERR_FILENO */
#include <errno.h>  /* errno */
#include <signal.h> /* signal */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "net.h"

#define MAX_BUF_SIZE 2048

/** パイプ */
enum {
    PIPE_R = 0, /**< リード */
    PIPE_W,     /**< ライト */
    MAX_PIPE    /**< パイプ数 */
};

/* プロトタイプ */
/** system_log() 関数テスト */
void test_system_log(void);
/** system_dbg_log() 関数テスト */
void test_system_dbg_log(void);
/** stderr_log() 関数テスト */
void test_stderr_log(void);
/** dump_log() 関数テスト */
void test_dump_log(void);
/** dump_sys() 関数テスト */
void test_dump_sys(void);
/** dump_file() 関数テスト */
void test_dump_file(void);
/** print_trace() 関数テスト */
void test_print_trace(void);

/* 内部変数 */
static char dump[0xFF + 1];     /**< ダンプデータ */
static const int EX_ERROR = -1; /**< エラー戻り値 */

/* 内部関数 */
/** 標準エラー出力用文字列設定 */
static void set_print_hex(char *data);
/** シスログ出力用文字列設定 */
static void set_print_hex_sys(char *data, const char *prefix);
/** リダイレクト */
static int pipe_fd(const int fd);
/** ファイルディスクリプタクローズ */
static int close_fd(int *fd);
/** シグナル設定 */
static void set_sig_handler(void);

/** ダンプ表示文字列 */
const char *print_hex[] = {
    "00000000 : 0001 0203 0405 0607 0809 0a0b 0c0d 0e0f ................",
    "00000010 : 1011 1213 1415 1617 1819 1a1b 1c1d 1e1f ................",
    "00000020 : 2021 2223 2425 2627 2829 2a2b 2c2d 2e2f  !\"#$%&'()*+,-./",
    "00000030 : 3031 3233 3435 3637 3839 3a3b 3c3d 3e3f 0123456789:;<=>?",
    "00000040 : 4041 4243 4445 4647 4849 4a4b 4c4d 4e4f @ABCDEFGHIJKLMNO",
    "00000050 : 5051 5253 5455 5657 5859 5a5b 5c5d 5e5f PQRSTUVWXYZ[\\]^_",
    "00000060 : 6061 6263 6465 6667 6869 6a6b 6c6d 6e6f `abcdefghijklmno",
    "00000070 : 7071 7273 7475 7677 7879 7a7b 7c7d 7e7f pqrstuvwxyz{|}~.",
    "00000080 : 8081 8283 8485 8687 8889 8a8b 8c8d 8e8f ................",
    "00000090 : 9091 9293 9495 9697 9899 9a9b 9c9d 9e9f ................",
    "000000A0 : a0a1 a2a3 a4a5 a6a7 a8a9 aaab acad aeaf ................",
    "000000B0 : b0b1 b2b3 b4b5 b6b7 b8b9 babb bcbd bebf ................",
    "000000C0 : c0c1 c2c3 c4c5 c6c7 c8c9 cacb cccd cecf ................",
    "000000D0 : d0d1 d2d3 d4d5 d6d7 d8d9 dadb dcdd dedf ................",
    "000000E0 : e0e1 e2e3 e4e5 e6e7 e8e9 eaeb eced eeef ................",
    "000000F0 : f0f1 f2f3 f4f5 f6f7 f8f9 fafb fcfd feff ................",
};

/**
 * 初期化処理
 *
 * @return なし
 */
void cut_startup(void)
{
    set_sig_handler();

    char hex = 0x00; /* 16進数 */
    (void)memset(dump, 0, sizeof(dump));

    uint i;
    for (i = 0; i < sizeof(dump); i++) {
        dump[i] = hex++;
    }
}

/**
 * system_log() 関数テスト
 *
 * @return なし
 */
void
test_system_log(void)
{
    int fd = -1;                     /* ファイル記述子 */
    int retval = 0;                  /* 戻り値 */
    char actual[MAX_BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =          /* 期待する文字列 */
        "programname\\[[0-9]+\\]: ppid=[0-9]+, tid=[0-9]+: " \
        "filename\\[15\\]: function\\(test\\): (.*)\\([0-9]+\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }

    system_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname",
               "filename", 15, "function", "%s", "test");

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }
    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));

error_handler:
    retval = close_fd(&fd);
    if (retval < 0)
        cut_notify("close_fd=%d(%d)", retval, errno);
}

/**
 * system_dbg_log() 関数テスト
 *
 * @return なし
 */
void
test_system_dbg_log(void)
{
    int fd = -1;                     /* ファイル記述子 */
    int retval = 0;                  /* 戻り値 */
    char actual[MAX_BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =          /* 期待する文字列 */
        "programname\\[[0-9]+\\]: ppid=[0-9]+, tid=[0-9]+: " \
        "[0-9].\\.[0-9]+: filename\\[15\\]: " \
        "function\\(test\\): (.*)\\([0-9]+\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }

    system_dbg_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname",
                   "filename", 15, "function", "%s", "test");

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }

    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));

error_handler:
    retval = close_fd(&fd);
    if (retval < 0)
        cut_notify("close_fd=%d(%d)", retval, errno);
}

/**
 * stderr_log() 関数テスト
 *
 * @return なし
 */
void
test_stderr_log(void)
{
    int fd = -1;                     /* ファイル記述子 */
    int retval = 0;                  /* 戻り値 */
    char actual[MAX_BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =          /* 期待する文字列 */
        "[A-Z][a-z]. [ 0-9][0-9] [0-9].:[0-9].:[0-9].\\.[0-9]+ " \
        "[A-Za-z]+ programname\\[[0-9]+\\]: filename\\[15\\]: " \
        "ppid=[0-9]+, tid=[0-9]+: function\\(test\\): (.*)\\([0-9]+\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }

    stderr_log("programname", "filename", 15, "function", "%s", "test");

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }

    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));

error_handler:
    retval = close_fd(&fd);
    if (retval < 0)
        cut_notify("close_fd=%d(%d)", retval, errno);
}

/**
 * dump_log() 関数テスト
 *
 * @return なし
 */
void
test_dump_log(void)
{
    int fd = -1;                       /* ファイル記述子 */
    int retval = 0;                    /* 戻り値 */
    int result_ok = 0;                 /* テスト関数戻り値 */
    char expected[MAX_BUF_SIZE] = {0}; /* 期待する文字列 */
    char actual[MAX_BUF_SIZE] = {0};   /* 実際の文字列 */
    char tmp[MAX_BUF_SIZE] = {0};      /* 一時バッファ */

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }
    result_ok = dump_log(dump, sizeof(dump), "%s[%d]: %s(%s)",
                         "filename", 15, "function", "test");

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }

    set_print_hex(tmp);
    (void)snprintf(expected, sizeof(expected), "%s%s",
                   "filename[15]: function(test)\n" \
                   "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F " \
                   "0123456789ABCDEF\n" \
                   "--------   ---- ---- ---- ---- ---- ---- ---- ---- " \
                   "----------------\n", tmp);
    cut_assert_equal_string(expected, actual,
                            cut_message("expected=%s actual=%s",
                                        expected, actual));

    cut_assert_equal_int(EX_OK, result_ok, cut_message("return value"));

    /* 異常系 */
    result_ok = dump_log(NULL, 0, "%s[%d]: %s(%s)",
                         "filename", 15, "function", "test");

    cut_assert_equal_int(EX_NG, result_ok, cut_message("return value"));

error_handler:
    retval = close_fd(&fd);
    if (retval < 0)
        cut_notify("close_fd=%d(%d)", retval, errno);
}

/**
 * dump_sys() 関数テスト
 *
 * @return なし
 */
void
test_dump_sys(void)
{

    int fd = -1;                       /* ファイル記述子 */
    int retval = 0;                    /* 戻り値 */
    int result_ok = 0;                 /* テスト関数戻り値 */
    char expected[MAX_BUF_SIZE] = {0}; /* 期待する文字列 */
    char actual[MAX_BUF_SIZE] = {0};   /* 実際の文字列 */
    const char prefix[] =              /* プレフィックス */
        "programname\\[[0-9]+\\]: filename\\[15\\]: " \
        "function\\(test\\): ";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }

    result_ok = dump_sys(LOG_INFO, LOG_PID | LOG_PERROR,
                         "programname", "filename", 15,
                         "function", dump, sizeof(dump), "%s", "test");

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }

    set_print_hex_sys(expected, prefix);
    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));

    cut_assert_equal_int(EX_OK, result_ok, cut_message("return value"));

    /* 異常系 */
    result_ok = dump_sys(LOG_INFO, LOG_PID | LOG_PERROR,
                         "programname", "filename", 15,
                         "function", NULL, 0, "%s", "test");

    cut_assert_equal_int(EX_NG, result_ok, cut_message("return value"));

error_handler:
    retval = close_fd(&fd);
    if (retval < 0)
        cut_notify("close_fd=%d(%d)", retval, errno);
}

/**
 * dump_file() 関数テスト
 *
 * @return なし
 */
void
test_dump_file(void)
{
    char testfile[L_tmpnam] = {0};     /* 一意なファイル名 */
    char *tmpret = NULL;               /* tmpnam戻り値 */
    char readbuf[0xFF + 1] = {0};      /* readバッファ */
    FILE *fp = NULL;                   /* ファイルポインタ */
    size_t rret = 0;                   /* fread戻り値 */
    int retval = 0;                    /* 戻り値 */
    int result_ok = 0;                 /* テスト関数戻り値 */

    /* 正常系 */
    tmpret = tmpnam(testfile);
    if (!tmpret) {
        cut_error("tmpnam=%p(%d)", tmpret, errno);
        return;
    }

    result_ok = dump_file("program", testfile, dump, sizeof(dump));

    fp = fopen(testfile, "rb");
    if (!fp) {
        cut_fail("fopen=%p(%d)", fp, errno);
        return;
    }

    rret = fread(readbuf, sizeof(readbuf), 1, fp);
    if (rret != 1) {
        cut_fail("fread=%p(%d)", fp, errno);
        goto error_handler;
    }

    retval = fflush(fp);
    if (retval == EOF)
        cut_notify("fflush=%d(%d)", retval, errno);

    cut_assert_equal_memory(dump, sizeof(dump),
                            readbuf, sizeof(readbuf));

    cut_assert_equal_int(EX_OK, result_ok, cut_message("return value"));

    /* 異常系 */
    result_ok = dump_file("program", testfile, NULL, 0);

    cut_assert_equal_int(EX_NG, result_ok, cut_message("return value"));

error_handler:
    retval = fclose(fp);
    if (retval == EOF)
        cut_notify("fclose=%d(%d)", retval, errno);

    retval = unlink(testfile);
    if (retval < 0)
        cut_notify("unlink=%d(%d)", retval, errno);
}

/**
 * print_trace() 関数テスト
 *
 * @return なし
 */
void
test_print_trace(void)
{
    int fd = -1;                     /* ファイル記述子 */
    int retval = 0;                  /* 戻り値 */
    char actual[MAX_BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =          /* 期待する文字列 */
        "Obtained [0-9]+ stack frames.\\n" \
        "0x[0-9a-f]+, *";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }

    print_trace();

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }

    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));

error_handler:
    retval = close_fd(&fd);
    if (retval < 0)
        cut_notify("close_fd=%d(%d)", retval, errno);
}

/**
 * 標準エラー出力用文字列設定
 *
 * @param[in,out] buf バッファ
 * @return なし
 */
static void
set_print_hex(char *buf)
{
    size_t length = 0; /* 文字列長(一行) */
    size_t total = 0;  /* 文字列長(全て) */

    int i;
    for (i = 0; i < NELEMS(print_hex); i++) {
        length = strlen(print_hex[i]);
        strncpy(buf + total, print_hex[i], length);
        total += length;
        strncpy(buf + total, "\n", strlen("\n"));
        total += strlen("\n");
    }
    strncpy(buf + total, "\n", strlen("\n"));
}

/**
 * シスログ出力用文字列設定
 *
 * @param[in,out] buf バッファ
 * @param[in] prefix プレフィックス
 * @return なし
 */
static void
set_print_hex_sys(char *buf, const char *prefix)
{
    size_t length = 0; /* 文字列長(一行) */
    size_t total = 0;  /* 文字列長(全て) */

    int i;
    const size_t prefix_len = strlen(prefix);
    for (i = 0; i < NELEMS(print_hex); i++) {
        strncpy(buf + total, prefix, prefix_len);
        total += prefix_len;
        length = strlen(print_hex[i]);
        strncpy(buf + total, print_hex[i], length);
        total += length;
        strncpy(buf + total, "\\n", strlen("\\n"));
        total += strlen("\\n");
    }
    *(buf + total - strlen("\\n")) = '\0'; /* 改行削除 */
}

/**
 * リダイレクト
 *
 * @param[in] fd ファイルディスクリプタ
 * @retval EX_ERROR エラー
 * @return ファイルディスクリプタ
 */
static int
pipe_fd(const int fd)
{
    int pfd[MAX_PIPE]; /* pipe */
    int retval = 0;    /* 戻り値 */

    retval = pipe(pfd);
    if (retval < 0) {
        outlog("pipe=%d", retval);
        return EX_ERROR;
    }

    retval = dup2(pfd[PIPE_W], fd);
    if (retval < 0) {
        outlog("dup2=%d", retval);
        return EX_ERROR;
    }

    retval = close(pfd[PIPE_W]);
    if (retval < 0) {
        outlog("close=%d, fd=%d", retval, fd);
        return EX_ERROR;
    }
    return pfd[PIPE_R];
}

/**
 * ファイルディスクリプタクローズ
 *
 * @param[in] fd ファイルディスクリプタ
 * @retval EX_NG エラー
 */
static int
close_fd(int *fd)
{
    int retval = 0; /* 戻り値 */

    dbglog("start: fd=%d", *fd);

    retval = close(*fd);
    if (retval < 0) {
        outlog("close=%d, fd=%d", retval, *fd);
        return EX_NG;
    }
    *fd = -1;
    return EX_OK;
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
        outlog("SIGINT");
    if (signal(SIGTERM, SIG_IGN) < 0)
        outlog("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) < 0)
        outlog("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) < 0)
        outlog("SIGHUP");
    if (signal(SIGCHLD, SIG_IGN) < 0)
        outlog("SIGCHLD");
    if (signal(SIGALRM, SIG_IGN) < 0)
        outlog("SIGALRM");
}


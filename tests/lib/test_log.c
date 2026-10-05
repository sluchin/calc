/**
 * @file tests/lib/test_log.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-11-19 higashi 新規作成
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

#include <stdio.h>    /* snprintf */
#include <stdlib.h>   /* free */
#include <execinfo.h> /* backtrace_symbols */
#include <dirent.h>   /* opendir readdir */
#include <sys/time.h> /* gettimeofday */
#include <time.h>     /* localtime_r */
#include <stdarg.h>   /* va_list */
#include <unistd.h>   /* STDERR_FILENO */
#include <fcntl.h>    /* open */
#include <errno.h>    /* errno */
#include <signal.h>   /* signal */

#include "test_helper.h"

#include "def.h"
#include "fileio.h"
#include "log.h"

#define BUF_SIZE 2048u /**< バッファサイズ */

DEFINE_FFF_GLOBALS

/* 標準ライブラリの関数は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
/* va_list の引数は, FFF が引数を保存するとき, 配列と見なされて警告される */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-array-argument"
FAKE_VALUE_FUNC(int, vsnprintf, char *, size_t, const char *, va_list)
TEST_PASSTHROUGH(int,
                 vsnprintf,
                 (char *str, size_t size, const char *format, va_list ap),
                 (str, size, format, ap))
FAKE_VALUE_FUNC(int, vfprintf, FILE *, const char *, va_list)
TEST_PASSTHROUGH(int, vfprintf, (FILE * fp, const char *format, va_list ap), (fp, format, ap))
FAKE_VALUE_FUNC(int, gettimeofday, struct timeval *, void *)
TEST_PASSTHROUGH(int, gettimeofday, (struct timeval * tv, void *tz), (tv, tz))
FAKE_VALUE_FUNC(struct tm *, localtime_r, const time_t *, struct tm *)
TEST_PASSTHROUGH(struct tm *,
                 localtime_r,
                 (const time_t *timep, struct tm *result),
                 (timep, result))
FAKE_VALUE_FUNC(int, fclose, FILE *)
TEST_PASSTHROUGH(int, fclose, (FILE * fp), (fp))
FAKE_VALUE_FUNC(int, gethostname, char *, size_t)
TEST_PASSTHROUGH(int, gethostname, (char *name, size_t len), (name, len))
#pragma GCC diagnostic pop
#ifdef HAVE_EXECINFO
FAKE_VALUE_FUNC(char **, backtrace_symbols, void *const *, int)
TEST_PASSTHROUGH(char **, backtrace_symbols, (void *const *buffer, int size), (buffer, size))
#endif

/* プロトタイプ */
TEST test_set_progname(void);
TEST test_get_progname(void);
TEST test_system_log(void);
TEST test_system_dbg_log(void);
TEST test_stderr_log(void);
TEST test_dump_log(void);
TEST test_dump_sys(void);
TEST test_dump_file(void);
TEST test_system_log_failure(void);
TEST test_system_dbg_log_failure(void);
TEST test_stderr_log_failure(void);
TEST test_dump_log_failure(void);
TEST test_dump_sys_failure(void);
TEST test_dump_file_failure(void);
#ifdef HAVE_EXECINFO
TEST test_systrace_failure(void);
#endif
#ifdef HAVE_EXECINFO
TEST test_systrace(void);
TEST test_print_trace(void);
#endif

/* 内部変数 */
static char dump[0xFF + 1];                    /**< ダンプデータ */
static int fd = -1;                            /**< ファイルディスクリプタ */
static char testfile[TEST_TMPNAME_SIZE] = {0}; /**< 一意なファイル名 */

/* 内部関数 */
static void set_print_hex(char *buf, size_t len);
static int match_print_hex_sys(const char *actual, const char *prefix);
static void set_sig_handler(void);

/** ダンプ表示文字列 */
static const char *print_hex[] = {
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
 */
static void
startup(void)
{
    /* 初期化 (シグナルの設定など) */
    (void)signal(SIGPIPE, SIG_IGN);
    set_sig_handler();

    char hex = 0x00; /* 16進数 */
    (void)memset(dump, 0, sizeof(dump));

    unsigned int i;
    for (i = 0u; i < sizeof(dump); i++) {
        dump[i] = hex++;
    }
}

/**
 * 初期化処理
 *
 * @param[in] data 使用しない
 */
static void
setup(void *data)
{
    /* モックと状態を, 初期状態 (素通し) に戻す */
    (void)data;
    TEST_PASSTHROUGH_RESET(vsnprintf);
    TEST_PASSTHROUGH_RESET(vfprintf);
    TEST_PASSTHROUGH_RESET(gettimeofday);
    TEST_PASSTHROUGH_RESET(localtime_r);
    TEST_PASSTHROUGH_RESET(gethostname);
    TEST_PASSTHROUGH_RESET(fclose);
#ifdef HAVE_EXECINFO
    TEST_PASSTHROUGH_RESET(backtrace_symbols);
#endif
    FFF_RESET_HISTORY();
}

/**
 * 終了処理
 *
 * @param[in] data 使用しない
 */
static void
teardown(void *data)
{
    (void)data; /* 使用しない */
    if (fd != -1) {
        if (close(fd) < 0)
            TEST_NOTIFY("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }

    if (testfile[0] != '\0') {
        if (access(testfile, W_OK) == 0) { /* ファイルが存在する */
            if (unlink(testfile) < 0)
                TEST_NOTIFY("unlink: %s(%d)", testfile, errno);
        }
        (void)memset(testfile, 0, sizeof(testfile));
    }
}

/**
 * set_progname() 関数テスト
 */
TEST
test_set_progname(void)
{
    char *ptr = NULL;                   /* テスト関数戻り値 */
    const char *prog = "/tmp/testprog"; /* テストデータ */

    set_progname(prog);
    ptr = get_progname();
    TEST_ASSERT_STR("testprog", ptr);
    PASS();
}

/**
 * get_progname() 関数テスト
 */
TEST
test_get_progname(void)
{
    char *ptr = NULL;              /* テスト関数戻り値 */
    const char *prog = "testprog"; /* テストデータ */

    set_progname(prog);
    ptr = get_progname();
    TEST_ASSERT_STR("testprog", ptr);
    PASS();
}

/**
 * system_log() 関数テスト
 */
TEST
test_system_log(void)
{
    ssize_t rlen = 0L;           /* read戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "programname\\[[0-9]+\\]: ppid=[0-9]+, tid=[0-9]+: "
        "filename\\[15\\]: function\\(test\\): (.*)\\([0-9]+\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd=%d(%d)", fd, errno);
    }

    system_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function", "%s",
               "test");

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }
    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s", expected, actual);
    PASS();
}

/**
 * system_dbg_log() 関数テスト
 */
TEST
test_system_dbg_log(void)
{
    ssize_t rlen = 0L;           /* read戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "programname\\[[0-9]+\\]: ppid=[0-9]+, tid=[0-9]+: "
        "[0-9].\\.[0-9]+: filename\\[15\\]: "
        "function\\(test\\): (.*)\\([0-9]+\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd=%d(%d)", fd, errno);
    }

    system_dbg_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function", "%s",
                   "test");

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s", expected, actual);
    PASS();
}

/**
 * stderr_log() 関数テスト
 */
TEST
test_stderr_log(void)
{
    ssize_t rlen = 0L;           /* 戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "[A-Z][a-z]. [ 0-9][0-9] [0-9].:[0-9].:[0-9].\\.[0-9]+ "
        ".* programname\\[[0-9]+\\]: filename\\[15\\]: "
        "ppid=[0-9]+, tid=[0-9]+: function\\(test\\): (.*)\\([0-9]+\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd=%d(%d)", fd, errno);
    }

    stderr_log("programname", "filename", 15, "function", "%s", "test");

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }
    dbglog("actual=%s", actual);

    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s", expected, actual);
    PASS();
}

/**
 * dump_log() 関数テスト
 */
TEST
test_dump_log(void)
{
    ssize_t rlen = 0L;                 /* read戻り値 */
    int result_ok = 0;                 /* テスト関数戻り値 */
    char expected[BUF_SIZE * 2] = {0}; /* 期待する文字列 (tmp + ヘッダより大きく) */
    char actual[BUF_SIZE] = {0};       /* 実際の文字列 */
    char tmp[BUF_SIZE] = {0};          /* 一時バッファ */

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd(%d)", errno);
    }
    result_ok = dump_log(dump, sizeof(dump), "%s[%d]: %s(%s)", "filename", 15, "function", "test");

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    set_print_hex(tmp, sizeof(tmp));
    (void)snprintf(expected, sizeof(expected), "%s%s",
                   "filename[15]: function(test)\n"
                   "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F "
                   "0123456789ABCDEF\n"
                   "--------   ---- ---- ---- ---- ---- ---- ---- ---- "
                   "----------------\n",
                   tmp);
    TEST_ASSERT_STR_MSG(expected, actual, "expected=%s actual=%s", expected, actual);

    TEST_ASSERT_INT_MSG(EX_OK, result_ok, "return value");

    /* 異常系 */
    result_ok = dump_log(NULL, 0u, "%s[%d]: %s(%s)", "filename", 15, "function", "test");

    TEST_ASSERT_INT_MSG(EX_NG, result_ok, "return value");
    PASS();
}

/**
 * dump_sys() 関数テスト
 */
TEST
test_dump_sys(void)
{
    ssize_t rlen = 0L;           /* read戻り値 */
    int result_ok = 0;           /* テスト関数戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char prefix[] =        /* プレフィックス */
        "programname\\[[0-9]+\\]: filename\\[15\\]: "
        "function\\(test\\): ";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd(%d)", errno);
    }

    result_ok = dump_sys(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function",
                         dump, sizeof(dump), "%s", "test");

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    TEST_ASSERT_MSG(match_print_hex_sys(actual, prefix), "actual=%s", actual);

    TEST_ASSERT_INT_MSG(EX_OK, result_ok, "return value");

    /* 異常系 */
    result_ok = dump_sys(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function",
                         NULL, 0u, "%s", "test");

    TEST_ASSERT_INT_MSG(EX_NG, result_ok, "return value");
    PASS();
}

/**
 * dump_file() 関数テスト
 */
TEST
test_dump_file(void)
{
    char readbuf[0xFF + 1] = {0}; /* readバッファ */
    ssize_t rlen = 0L;            /* read戻り値 */
    int result_ok = 0;            /* テスト関数戻り値 */

    /* 正常系 */
    if (test_tmpname(testfile) < 0) {
        TEST_FAIL("test_tmpname(%d)", errno);
    }

    result_ok = dump_file("program", testfile, dump, sizeof(dump));

    fd = open(testfile, O_RDONLY);
    if (fd < 0) {
        TEST_FAIL("open(%d)", errno);
    }

    rlen = read(fd, readbuf, sizeof(readbuf));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    TEST_ASSERT_MEM(dump, sizeof(dump), readbuf, sizeof(readbuf));

    TEST_ASSERT_INT_MSG(EX_OK, result_ok, "return value");

    /* 異常系 */
    result_ok = dump_file("program", testfile, NULL, 0u);

    TEST_ASSERT_INT_MSG(EX_NG, result_ok, "return value");

    PASS();
}

#ifdef HAVE_EXECINFO
/**
 * systrace() 関数テスト
 */
TEST
test_systrace(void)
{
    ssize_t rlen = 0L;           /* 戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "programname\\[[0-9]+\\]: filename\\[15\\]: function: "
        "Obtained [0-9]+ stack frames.\\n*";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd(%d)", errno);
    }

    systrace(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function");

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s", expected, actual);
    PASS();
}

/**
 * print_trace() 関数テスト
 */
TEST
test_print_trace(void)
{
    ssize_t rlen = 0L;           /* 戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "Obtained [0-9]+ stack frames.\\n*";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd(%d)", errno);
    }

    print_trace();

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s", expected, actual);
    PASS();
}
#endif

/**
 * 標準エラー出力用文字列設定
 *
 * @param[in,out] buf バッファ
 * @param[in] len バッファサイズ
 */
static void
set_print_hex(char *buf, size_t len)
{
    size_t length = 0u; /* 文字列長(一行) */
    size_t total = 0u;  /* 文字列長(全て) */

    unsigned int i;
    for (i = 0u; i < NELEMS(print_hex); i++) {
        length = strlen(print_hex[i]);
        strncat(buf, print_hex[i], len - total - 1u);
        total += length;
        strncat(buf, "\n", len - total - 1u);
        total += strlen("\n");
    }
    (void)strcat(buf, "\n");
}

/**
 * シスログ出力の確認
 * ダンプ表示は文字列として一致を確認し, 行頭のプレフィックスだけを正規表現で
 * 確認する (ダンプ表示に正規表現の特殊文字が含まれるため).
 *
 * @param[in] actual 実際の文字列
 * @param[in] prefix プレフィックス (正規表現)
 * @retval 1 一致
 * @retval 0 不一致
 */
static int
match_print_hex_sys(const char *actual, const char *prefix)
{
    char pattern[BUF_SIZE] = {0}; /* プレフィックス用正規表現 */
    char head[BUF_SIZE] = {0};    /* 実際のプレフィックス */
    const char *line = actual;    /* 行頭 */
    const char *pos = NULL;       /* ダンプ表示の位置 */
    const char *lf = NULL;        /* 直前の改行 */

    (void)snprintf(pattern, sizeof(pattern), "^%s$", prefix);

    unsigned int i;
    for (i = 0u; i < NELEMS(print_hex); i++) {
        pos = strstr(line, print_hex[i]);
        if (pos == NULL)
            return 0;
        /* 直前の改行の次から, ダンプ表示の前までがプレフィックス */
        lf = pos;
        while ((lf > actual) && (*(lf - 1) != '\n'))
            lf--;
        if ((size_t)(pos - lf) >= sizeof(head))
            return 0;
        (void)memcpy(head, lf, (size_t)(pos - lf));
        head[pos - lf] = '\0';
        if (test_match(pattern, head) == 0)
            return 0;
        line = pos + strlen(print_hex[i]);
    }
    return 1;
}

/**
 * シグナル設定
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGINT");
    if (signal(SIGTERM, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGHUP");
    if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGALRM");
}

/**
 * 失敗のログが標準エラー出力 (前のテストで閉じたパイプ) に出ないように,
 * /dev/null に向ける
 */
static void
quiet_stderr(void)
{
    (void)redirect(STDERR_FILENO, "/dev/null");
}

/**
 * system_log() 関数テスト (失敗)
 */
TEST
test_system_log_failure(void)
{
    quiet_stderr();

    /* vsnprintf() に失敗 */
    TEST_INJECT(vsnprintf, 0, 1, -1, EILSEQ);
    system_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function", "%s",
               "test");
    TEST_ASSERT_INJECTED(vsnprintf);
    PASS();
}

/**
 * system_dbg_log() 関数テスト (失敗)
 */
TEST
test_system_dbg_log_failure(void)
{
    quiet_stderr();

    /* gettimeofday() に失敗 */
    TEST_INJECT(gettimeofday, 0, 1, -1, EFAULT);
    system_dbg_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function", "%s",
                   "test");
    TEST_ASSERT_INJECTED(gettimeofday);

    /* localtime_r() に失敗 */
    TEST_INJECT(localtime_r, 0, 1, NULL, EOVERFLOW);
    system_dbg_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function", "%s",
                   "test");
    TEST_ASSERT_INJECTED(localtime_r);

    /* vsnprintf() に失敗 */
    TEST_INJECT(vsnprintf, 0, 1, -1, EILSEQ);
    system_dbg_log(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function", "%s",
                   "test");
    TEST_ASSERT_INJECTED(vsnprintf);
    PASS();
}

/**
 * stderr_log() 関数テスト (失敗)
 */
TEST
test_stderr_log_failure(void)
{
    quiet_stderr();

    /* gettimeofday() に失敗 */
    TEST_INJECT(gettimeofday, 0, 1, -1, EFAULT);
    stderr_log("programname", "filename", 15, "function", "%s", "test");
    TEST_ASSERT_INJECTED(gettimeofday);

    /* localtime_r() に失敗 */
    TEST_INJECT(localtime_r, 0, 1, NULL, EOVERFLOW);
    stderr_log("programname", "filename", 15, "function", "%s", "test");
    TEST_ASSERT_INJECTED(localtime_r);

    /* gethostname() に失敗 */
    TEST_INJECT(gethostname, 0, 1, -1, EFAULT);
    stderr_log("programname", "filename", 15, "function", "%s", "test");
    TEST_ASSERT_INJECTED(gethostname);

    /* vfprintf() に失敗 */
    TEST_INJECT(vfprintf, 0, 1, -1, EILSEQ);
    stderr_log("programname", "filename", 15, "function", "%s", "test");
    TEST_ASSERT_INJECTED(vfprintf);
    PASS();
}

/**
 * dump_log() 関数テスト (失敗)
 */
TEST
test_dump_log_failure(void)
{
    quiet_stderr();

    /* vsnprintf() に失敗 */
    TEST_INJECT(vsnprintf, 0, 1, -1, EILSEQ);
    TEST_ASSERT_INT(EX_NG, dump_log(dump, sizeof(dump), "%s", "test"));
    TEST_ASSERT_INJECTED(vsnprintf);

    /* 16 バイトに満たない行を, 空白で埋める */
    TEST_ASSERT_INT(EX_OK, dump_log(dump, 3u, "%s", "test"));
    PASS();
}

/**
 * dump_sys() 関数テスト (失敗)
 */
TEST
test_dump_sys_failure(void)
{
    quiet_stderr();

    /* vsnprintf() に失敗 */
    TEST_INJECT(vsnprintf, 0, 1, -1, EILSEQ);
    TEST_ASSERT_INT(EX_NG, dump_sys(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15,
                                    "function", dump, sizeof(dump), "%s", "test"));
    TEST_ASSERT_INJECTED(vsnprintf);

    /* 16 バイトに満たない行を, 空白で埋める */
    TEST_ASSERT_INT(EX_OK, dump_sys(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15,
                                    "function", dump, 3u, "%s", "test"));
    PASS();
}

/**
 * オープンしているファイルディスクリプタの数を数える (リークの確認用)
 *
 * @return ファイルディスクリプタの数
 */
static int
count_open_fds(void)
{
    int count = 0;                       /* 数 */
    DIR *dir = opendir("/proc/self/fd"); /* ディレクトリ */
    struct dirent *ent = NULL;           /* ディレクトリエントリ */

    if (dir == NULL)
        return -1;
    ent = readdir(dir);
    while (ent != NULL) {
        count++;
        ent = readdir(dir);
    }
    (void)closedir(dir);
    return count;
}

/**
 * dump_file() 関数テスト (失敗)
 */
TEST
test_dump_file_failure(void)
{
    char big[BUF_SIZE * 4u]; /* stdio のバッファより大きいデータ */
    int fds = 0;             /* オープンしているファイルディスクリプタの数 */

    quiet_stderr();
    (void)memset(big, 'a', sizeof(big));

    /* オープンできない */
    TEST_ASSERT_INT(EX_NG, dump_file("program", "/nonexistent/dir/file", "abc", 3u));
    /* 書込に失敗 (/dev/full は, 常に ENOSPC になる). ファイルは閉じる */
    fds = count_open_fds();
    TEST_ASSERT_INT(EX_NG, dump_file("program", "/dev/full", big, sizeof(big)));
    TEST_ASSERT_INT(fds, count_open_fds());
    /* バッファに収まるデータは, fwrite では失敗せず, fflush で失敗する
     * (ログを出力するだけで, 戻り値は変わらない) */
    TEST_ASSERT_INT(EX_OK, dump_file("program", "/dev/full", "abc", 3u));
    /* fclose() に失敗 */
    TEST_INJECT(fclose, 0, 1, EOF, EIO);
    TEST_ASSERT_INT(EX_NG, dump_file("program", "/dev/null", "abc", 3u));
    TEST_ASSERT_INJECTED(fclose);
    PASS();
}

#ifdef HAVE_EXECINFO
/**
 * systrace() 関数テスト (失敗)
 */
TEST
test_systrace_failure(void)
{
    quiet_stderr();

    /* backtrace_symbols() に失敗 */
    TEST_INJECT(backtrace_symbols, 0, 1, NULL, ENOMEM);
    systrace(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function");
    TEST_ASSERT_INJECTED(backtrace_symbols);
    PASS();
}
#endif

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
    /* 全てのテストの前に, 1 回だけ行う初期化 */
    startup();
    /* 各テストの前後に行う処理 */
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    /* テストの実行 */
    RUN_TEST(test_set_progname);
    RUN_TEST(test_get_progname);
    RUN_TEST(test_system_log);
    RUN_TEST(test_system_dbg_log);
    RUN_TEST(test_stderr_log);
    RUN_TEST(test_dump_log);
    RUN_TEST(test_dump_sys);
    RUN_TEST(test_dump_file);
    RUN_TEST(test_system_log_failure);
    RUN_TEST(test_system_dbg_log_failure);
    RUN_TEST(test_stderr_log_failure);
    RUN_TEST(test_dump_log_failure);
    RUN_TEST(test_dump_sys_failure);
    RUN_TEST(test_dump_file_failure);
    RUN_TEST(test_systrace);
    RUN_TEST(test_print_trace);
    RUN_TEST(test_systrace_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}

/**
 * @file tests/lib/test_fileio.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-26 higashi 新規作成
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
#include <unistd.h>   /* dup2 unlink pipe fork */
#include <fcntl.h>    /* open creat */
#include <sys/stat.h> /* chmod */
#include <sys/wait.h> /* wait waitpid */
#include <signal.h>   /* signal */
#include <errno.h>    /* errno */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "fileio.h"

#define BUF_SIZE 4100u /**< バッファサイズ */

DEFINE_FFF_GLOBALS

/* システムコールは, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(ssize_t, read, int, void *, size_t)
TEST_PASSTHROUGH(ssize_t, read, (int fd, void *buf, size_t n), (fd, buf, n))
FAKE_VALUE_FUNC(ssize_t, write, int, const void *, size_t)
TEST_PASSTHROUGH(ssize_t, write, (int fd, const void *buf, size_t n), (fd, buf, n))
/* pipe() は, 引数が配列 (int[2]) と宣言されているので, ポインタとの違いを警告される */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-parameter"
FAKE_VALUE_FUNC(int, pipe, int *)
TEST_PASSTHROUGH(int, pipe, (int *pipefd), (pipefd))
#pragma GCC diagnostic pop
FAKE_VALUE_FUNC(int, close, int)
TEST_PASSTHROUGH(int, close, (int fd), (fd))
FAKE_VALUE_FUNC(int, dup2, int, int)
TEST_PASSTHROUGH(int, dup2, (int oldfd, int newfd), (oldfd, newfd))

/* プロトタイプ */
/** readn() 関数テスト */
TEST test_readn(void);
/** writen() 関数テスト */
TEST test_writen(void);
/** pipe_fd() 関数テスト */
TEST test_pipe_fd(void);
/** redirect() 関数テスト */
TEST test_redirect(void);
/** close_fd() 関数テスト */
TEST test_close_fd(void);
/** readn() 関数テスト (失敗) */
TEST test_readn_failure(void);
/** readn() 関数テスト (EINTR) */
TEST test_readn_interrupted(void);
/** writen() 関数テスト (失敗) */
TEST test_writen_failure(void);
/** writen() 関数テスト (EINTR) */
TEST test_writen_interrupted(void);
/** pipe_fd() 関数テスト (失敗) */
TEST test_pipe_fd_failure(void);
/** pipe_fd2() 関数テスト (失敗) */
TEST test_pipe_fd2_failure(void);
/** redirect() 関数テスト (失敗) */
TEST test_redirect_failure(void);
/** close_fd() 関数テスト (失敗) */
TEST test_close_fd_failure(void);

/* 内部変数 */
static char testfile[TEST_TMPNAME_SIZE] = {0}; /**< 一意なファイル名 */
static int fd = -1;                            /**< ファイルディスクリプタ */
static int pfd[] = {-1, -1};                   /**< パイプ */
static char sendbuf[BUF_SIZE];                 /**< 送信バッファ */

/* 内部関数 */
/** 受信プロセス起動 */
static int read_child_process(char *readbuf, char *senddata, size_t len);
/** 送信プロセス起動 */
static int write_child_process(char *buf, size_t len);
/** シグナル設定 */
static void set_sig_handler(void);

/**
 * 初期化処理
 */
static void
startup(void)
{
    set_sig_handler();
    (void)memset(sendbuf, 'a', sizeof(sendbuf));
    sendbuf[sizeof(sendbuf) - 1u] = '\0';
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
    TEST_PASSTHROUGH_RESET(read);
    TEST_PASSTHROUGH_RESET(write);
    TEST_PASSTHROUGH_RESET(pipe);
    TEST_PASSTHROUGH_RESET(dup2);
    TEST_PASSTHROUGH_RESET(close);
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
    (void)data;     /* 使用しない */
    int retval = 0; /* 戻り値 */

    if (fd != -1) {
        if (close(fd) < 0)
            TEST_NOTIFY("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }
    if (pfd[PIPE_R] != -1) {
        if (close(pfd[PIPE_R]) < 0)
            TEST_NOTIFY("close: fd=%d(%d)", pfd[PIPE_R], errno);
        pfd[PIPE_R] = -1;
    }
    if (pfd[PIPE_W] != -1) {
        if (close(pfd[PIPE_W]) < 0)
            TEST_NOTIFY("close: fd=%d(%d)", pfd[PIPE_W], errno);
        pfd[PIPE_W] = -1;
    }

    if (testfile[0] != '\0') {
        if (access(testfile, W_OK) == 0) { /* ファイルが存在する */
            retval = chmod(testfile, S_IWUSR | S_IWGRP);
            if (retval < 0)
                TEST_NOTIFY("chmod: %s(%d)", testfile, errno);
            retval = unlink(testfile);
            if (retval < 0)
                TEST_NOTIFY("unlink: %s(%d)", testfile, errno);
        }
        (void)memset(testfile, 0, sizeof(testfile));
    }
}

/**
 * readn() 関数テスト
 */
TEST
test_readn(void)
{

    ssize_t rlen = 0L;             /* 受信バイト数 */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    fd = read_child_process(readbuf, sendbuf, sizeof(sendbuf));
    if (fd < 0) {
        TEST_FAIL("write_child_process(%d)", errno);
    }

    dbglog("parent");
    rlen = writen(STDERR_FILENO, sendbuf, sizeof(sendbuf));
    if (rlen < 0L) {
        TEST_FAIL("writen(%d)", errno);
    }

    w = waitpid(-1, &status, WNOHANG);
    if (w < 0)
        TEST_NOTIFY("wait(%d)", errno);
    dbglog("w=%d", (int)w);
    TEST_ASSERT_INT(EXIT_SUCCESS, WEXITSTATUS(status));
    PASS();
}

/**
 * writen() 関数テスト
 */
TEST
test_writen(void)
{
    ssize_t rlen = 0L;             /* 受信バイト数 */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    fd = write_child_process(sendbuf, sizeof(sendbuf));
    if (fd < 0) {
        TEST_FAIL("write_child_process(%d)", errno);
    }

    dbglog("parent");
    (void)memset(readbuf, 0, sizeof(readbuf));
    rlen = readn(fd, readbuf, sizeof(sendbuf));
    if (rlen < 0L) {
        TEST_FAIL("readn(%d)", errno);
    }
    TEST_ASSERT_STR(sendbuf, readbuf);

    w = waitpid(-1, &status, WNOHANG);
    if (w < 0)
        TEST_NOTIFY("wait(%d)", errno);
    dbglog("w=%d", (int)w);
    TEST_ASSERT_INT(EXIT_SUCCESS, WEXITSTATUS(status));
    PASS();
}

/**
 * pipe_fd() 関数テスト
 */
TEST
test_pipe_fd(void)
{
    ssize_t rlen = 0L;             /* 受信バイト数 */
    ssize_t wlen = 0L;             /* 送信バイト数 */
    pid_t cpid = 0;                /* 子プロセスID */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    TEST_ASSERT_MSG((fd) >= (0), "return value");

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) {
        dbglog("child");

        wlen = writen(STDERR_FILENO, sendbuf, sizeof(sendbuf));
        if (wlen < 0L) {
            outlog("write: fd=%d", STDERR_FILENO);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        (void)memset(readbuf, 0, sizeof(readbuf));
        rlen = readn(fd, readbuf, sizeof(sendbuf));
        if (rlen < 0L) {
            TEST_FAIL("read(%d)", errno);
        }
        TEST_ASSERT_STR(sendbuf, readbuf);

        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) != 0)
            TEST_FAIL("status=%d(%d)", WEXITSTATUS(status), errno);
    }

    /* 異常系 */
    /* -1の場合 */
    fd = pipe_fd(-1);
    TEST_ASSERT_INT_MSG(EX_NG, fd, "return value");
    /* 不正なファイルディスクリプタ */
    fd = pipe_fd(65535);
    TEST_ASSERT_INT_MSG(EX_NG, fd, "return value");
    PASS();
}

/**
 * dup_fd() 関数テスト
 */
TEST
test_pipe_fd2(void)
{
    int retval = 0;                /* 戻り値 */
    ssize_t rlen = 0L;             /* 受信バイト数 */
    ssize_t wlen = 0L;             /* 送信バイト数 */
    pid_t cpid = 0;                /* 子プロセスID */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */
    int oldfd = 0;                 /* 退避用 */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    retval = pipe(pfd);
    if (retval < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) {
        dbglog("child");

        oldfd = dup(STDIN_FILENO);
        if (oldfd < 0)
            TEST_NOTIFY("dup(%d)", errno);
        retval = pipe_fd2(&pfd[PIPE_R], &pfd[PIPE_W], STDIN_FILENO);
        if (retval < 0) {
            outlog("dup_fd");
            exit(EXIT_FAILURE);
        }
        wlen = writen(STDIN_FILENO, sendbuf, sizeof(sendbuf));
        if (wlen < 0L) {
            outlog("write");
            exit(EXIT_FAILURE);
        }
        dbglog("writen=%zd, %s", wlen, sendbuf);

        if (dup2(oldfd, STDIN_FILENO) < 0)
            TEST_NOTIFY("dup2(%d)", errno);

        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        oldfd = dup(STDIN_FILENO);
        if (oldfd < 0)
            TEST_NOTIFY("dup(%d)", errno);
        retval = pipe_fd2(&pfd[PIPE_W], &pfd[PIPE_R], STDIN_FILENO);
        if (retval < 0) {
            TEST_FAIL("dup_fd(%d)", errno);
        }

        (void)memset(readbuf, 0, sizeof(readbuf));
        rlen = readn(STDIN_FILENO, readbuf, sizeof(sendbuf));
        if (rlen < 0L) {
            TEST_FAIL("readn(%d)", errno);
        }
        dbglog("readn=%zd, %s", rlen, readbuf);

        TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");
        TEST_ASSERT_STR(sendbuf, readbuf);

        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) != 0)
            TEST_FAIL("status=%d(%d)", WEXITSTATUS(status), errno);
    }

    /* 異常系 */
    retval = pipe_fd2(&pfd[PIPE_W], &pfd[PIPE_R], STDIN_FILENO);
    TEST_ASSERT_INT_MSG(EX_NG, retval, "return value");

    if (dup2(oldfd, STDIN_FILENO) < 0)
        TEST_NOTIFY("dup2(%d)", errno);
    PASS();
}

/**
 * redirect() 関数テスト
 */
TEST
test_redirect(void)
{
    int retval = 0; /* 戻り値 */
    int oldfd = 0;  /* 退避用 */

    /* 正常系 */
    oldfd = dup(STDERR_FILENO);
    if (oldfd < 0)
        TEST_NOTIFY("dup(%d)", errno);
    retval = redirect(STDERR_FILENO, "/dev/null");
    TEST_ASSERT_INT_MSG(EX_OK, retval, "redirect");

    if (dup2(oldfd, STDERR_FILENO) < 0)
        TEST_NOTIFY("dup2(%d)", errno);

    /* 異常系 */
    /* ファイルディスクリプタが-1 */
    retval = redirect(-1, "/dev/null");
    TEST_ASSERT_INT_MSG(EX_NG, retval, "redirect: fd=-1");

    /* ファイルパスがNULL */
    retval = redirect(STDERR_FILENO, NULL);
    TEST_ASSERT_INT_MSG(EX_NG, retval, "redirect: path=null");

    /* 書込権限なし */
    if (test_tmpname(testfile) < 0) {
        TEST_FAIL("test_tmpname(%d)", errno);
    }

    retval = creat(testfile, S_IRUSR | S_IRGRP);
    if (retval < 0) {
        TEST_FAIL("create: %s(%d)", testfile, errno);
    }

    retval = redirect(STDERR_FILENO, testfile);
    TEST_ASSERT_INT_MSG(EX_NG, retval, "redirect: %s", testfile);
    PASS();
}

/**
 * close_fd() 関数テスト
 */
TEST
test_close_fd(void)
{
    int retval = 0;              /* 戻り値 */
    int fds[] = {-1, -1, -1};    /* ファイルディスクリプタ */
    enum { FD1, FD2, FD3, MAX }; /* 配列要素 */

    /* 正常系 */
    int i;
    for (i = 0; i < MAX; i++) {
        fds[i] = open("/dev/null", O_WRONLY | O_APPEND);
        if (fds[i] < 0) {
            TEST_FAIL("open=%d(%d)", fds[i], errno);
        }
    }
    retval = close_fd(&fds[FD1], &fds[FD2], &fds[FD3], NULL);
    TEST_ASSERT_INT(-1, fds[FD1]);
    TEST_ASSERT_INT(-1, fds[FD2]);
    TEST_ASSERT_INT(-1, fds[FD3]);
    TEST_ASSERT_INT_MSG(EX_OK, retval, "retrun value");

    /* 第二引数が-1の場合 */
    fds[FD1] = open("/dev/null", O_WRONLY | O_APPEND);
    if (fds[FD1] < 0) {
        TEST_FAIL("open=%d(%d)", fds[FD1], errno);
    }
    fds[FD3] = open("/dev/null", O_WRONLY | O_APPEND);
    if (fds[FD3] < 0) {
        TEST_FAIL("open=%d(%d)", fds[FD3], errno);
    }
    retval = close_fd(&fds[FD1], &fds[FD2], &fds[FD3], NULL);
    TEST_ASSERT_INT(-1, fds[FD1]);
    TEST_ASSERT_INT(-1, fds[FD3]);
    TEST_ASSERT_INT_MSG(EX_OK, retval, "retrun value");

    /* 異常系 */
    fds[FD1] = 65535; /* オープンしていない */
    retval = close_fd(&fds[FD1], NULL);
    TEST_ASSERT_INT(-1, fds[FD1]);
    TEST_ASSERT_INT_MSG(EX_NG, retval, "retrun value");
    PASS();
}

/**
 * 受信プロセス起動
 *
 * @param[out] readbuf 受信バッファ
 * @param[in] senddata 送信データ
 * @param[in] len バイト数
 * @return ファイルディスクリプタ
 * @retval EX_NG エラー
 */
static int
read_child_process(char *readbuf, char *senddata, size_t len)
{
    ssize_t rlen = 0L; /* 受信バイト数 */
    pid_t cpid = 0;    /* 子プロセスID */
    int f = 0;         /* ファイルディスクリプタ */
    int retval = 0;    /* 戻り値 */

    f = pipe_fd(STDERR_FILENO);
    if (f < 0) {
        TEST_ERROR("pipe_fd: fd=%d", STDERR_FILENO);
        return EX_NG;
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_ERROR("fork(%d)", errno);
        return EX_NG;
    }

    if (cpid == 0) {
        dbglog("child");

        rlen = readn(f, readbuf, len);
        if (rlen < 0L) {
            outlog("readn: fds=%d", f);
            close_fd(&f, NULL);
            exit(EXIT_FAILURE);
        }
        dbglog("readn=%zd", rlen);
        retval = memcmp(senddata, readbuf, len);
        if (retval != 0) { /* 非0 */
            outlog("memcmp: senddata=%p, readbuf=%p", senddata, readbuf);
            close_fd(&f, NULL);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    return f;
}

/**
 * 送信プロセス起動
 *
 * @param[out] buf バッファ
 * @param[in] len バイト数
 * @return ファイルディスクリプタ
 * @retval EX_NG エラー
 */
static int
write_child_process(char *buf, size_t len)
{
    ssize_t wlen = 0L; /* 送信バイト数 */
    pid_t cpid = 0;    /* 子プロセスID */
    int f = 0;         /* ファイルディスクリプタ */

    f = pipe_fd(STDERR_FILENO);
    if (f < 0) {
        TEST_ERROR("pipe_fd: fd=%d", STDERR_FILENO);
        return EX_NG;
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_ERROR("fork(%d)", errno);
        return EX_NG;
    }

    if (cpid == 0) {
        dbglog("child");

        wlen = writen(STDERR_FILENO, buf, len);
        if (wlen < 0L) {
            outlog("write: fd=%d", STDERR_FILENO);
            close_fd(&f, NULL);
            exit(EXIT_FAILURE);
        }
        dbglog("writen=%zd", wlen);
        exit(EXIT_SUCCESS);
    }

    return f;
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
 * オープンしていない, 小さなファイルディスクリプタ番号を探す
 * dup2() で作れるように, ファイルディスクリプタの上限 (ulimit -n) より小さくする.
 *
 * @return ファイルディスクリプタ番号
 * @retval -1 見つからない
 */
static int
unused_fd(void)
{
    int f; /* ファイルディスクリプタ */

    for (f = 100; f < 1000; f++) {
        if (fcntl(f, F_GETFD) < 0 && errno == EBADF)
            return f;
    }
    return -1;
}

/**
 * readn() 関数テスト (失敗)
 */
TEST
test_readn_failure(void)
{
    int p[2] = {-1, -1};   /* パイプ */
    char readbuf[8] = {0}; /* 受信バッファ */

    /* 不正なファイルディスクリプタ */
    TEST_ASSERT_INT(EX_NG, readn(-1, readbuf, sizeof(readbuf)));

    /* 途中で, 書込側が閉じられた (要求したバイト数より少ない) */
    if (pipe(p) < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }
    TEST_ASSERT_INT(2, write(p[1], "ab", 2u));
    (void)close(p[1]);
    TEST_ASSERT_INT(2, readn(p[0], readbuf, 4u));
    TEST_ASSERT_STR("ab", readbuf);
    (void)close(p[0]);
    PASS();
}

/**
 * readn() 関数テスト (EINTR)
 */
TEST
test_readn_interrupted(void)
{
    int p[2] = {-1, -1};   /* パイプ */
    char readbuf[8] = {0}; /* 受信バッファ */

    if (pipe(p) < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }
    TEST_ASSERT_INT(3, write(p[1], "abc", 3u));

    /* 1 回目は割り込まれるが, やり直して, 受信できる */
    TEST_INJECT(read, 0, 1, -1, EINTR);
    TEST_ASSERT_INT(3, readn(p[0], readbuf, 3u));
    TEST_ASSERT_INT(2, read_fake.call_count);
    TEST_ASSERT_STR("abc", readbuf);
    (void)close(p[0]);
    (void)close(p[1]);
    PASS();
}

/**
 * writen() 関数テスト (失敗)
 */
TEST
test_writen_failure(void)
{
    /* 不正なファイルディスクリプタ */
    TEST_ASSERT_INT(EX_NG, writen(-1, "abc", 3u));
    PASS();
}

/**
 * writen() 関数テスト (EINTR)
 */
TEST
test_writen_interrupted(void)
{
    int p[2] = {-1, -1};   /* パイプ */
    char readbuf[8] = {0}; /* 受信バッファ */

    if (pipe(p) < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }

    /* 1 回目は割り込まれるが, やり直して, 送信できる */
    TEST_INJECT(write, 0, 1, -1, EINTR);
    TEST_ASSERT_INT(3, writen(p[1], "abc", 3u));
    TEST_ASSERT_INT(2, write_fake.call_count);
    TEST_ASSERT_INT(3, read(p[0], readbuf, sizeof(readbuf)));
    TEST_ASSERT_STR("abc", readbuf);
    (void)close(p[0]);
    (void)close(p[1]);
    PASS();
}

/**
 * pipe_fd() 関数テスト (失敗)
 */
TEST
test_pipe_fd_failure(void)
{
    int newfd = -1; /* 置き換えるファイルディスクリプタ */

    /* pipe() に失敗 (標準エラー出力には触れない) */
    TEST_INJECT(pipe, 0, 1, -1, EMFILE);
    TEST_ASSERT_INT(EX_NG, pipe_fd(STDERR_FILENO));

    /* close() に失敗 (オープンしていない) */
    TEST_ASSERT_INT(EX_NG, pipe_fd(9999));

    /* dup2() に失敗 */
    newfd = dup(STDOUT_FILENO);
    if (newfd < 0) {
        TEST_FAIL("dup(%d)", errno);
    }
    TEST_INJECT(dup2, 0, 1, -1, EBADF);
    TEST_ASSERT_INT(EX_NG, pipe_fd(newfd));
    (void)close(newfd);
    PASS();
}

/**
 * pipe_fd2() 関数テスト (失敗)
 */
TEST
test_pipe_fd2_failure(void)
{
    int p[2] = {-1, -1};     /* パイプ */
    int oldfd = -1;          /* 退避用 */
    int newfd = unused_fd(); /* オープンしていない */
    int retval = 0;          /* 戻り値 */

    if (newfd < 0) {
        TEST_FAIL("unused_fd");
    }
    if (pipe(p) < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }
    oldfd = p[0];

    /* close() に失敗しても, dup2() できる */
    retval = pipe_fd2(&p[1], &oldfd, newfd);
    TEST_ASSERT_INT(newfd, retval);
    (void)close(newfd);
    PASS();
}

/**
 * redirect() 関数テスト (失敗)
 */
TEST
test_redirect_failure(void)
{
    int fdnum = unused_fd(); /* オープンしていない */

    if (fdnum < 0) {
        TEST_FAIL("unused_fd");
    }

    /* ディレクトリは, 書込権限があっても, open() できない */
    TEST_ASSERT_INT(EX_NG, redirect(STDERR_FILENO, "/tmp"));

    /* オープンしていないファイルディスクリプタ (close の失敗) */
    TEST_ASSERT_INT(EX_OK, redirect(fdnum, "/dev/null"));
    (void)close(fdnum);

    /* dup2() に失敗 */
    TEST_INJECT(dup2, 0, 1, -1, EBADF);
    TEST_ASSERT_INT(EX_NG, redirect(fdnum, "/dev/null"));

    /* 最後の close() に失敗しても, ログを出力するだけ */
    fdnum = open("/dev/null", O_RDWR);
    if (fdnum < 0) {
        TEST_FAIL("open(%d)", errno);
    }
    TEST_INJECT(close, 1, 1, -1, EIO); /* 1 回目は, 元のファイルディスクリプタ */
    TEST_ASSERT_INT(EX_OK, redirect(fdnum, "/dev/null"));
    TEST_ASSERT_INJECTED(close);
    (void)close(fdnum);
    PASS();
}

/**
 * close_fd() 関数テスト (失敗)
 */
TEST
test_close_fd_failure(void)
{
    int fds[] = {-1, 9999}; /* 2 つ目は, オープンしていない */

    fds[0] = open("/dev/null", O_WRONLY);
    if (fds[0] < 0) {
        TEST_FAIL("open(%d)", errno);
    }
    /* 可変引数の, 2 つ目の close() に失敗 */
    TEST_ASSERT_INT(EX_NG, close_fd(&fds[0], &fds[1], NULL));
    TEST_ASSERT_INT(-1, fds[0]);
    TEST_ASSERT_INT(-1, fds[1]);
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
    /* 全てのテストの前に, 1 回だけ行う初期化 */
    startup();
    /* 各テストの前後に行う処理 */
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    /* テストの実行 */
    RUN_TEST(test_readn);
    RUN_TEST(test_writen);
    RUN_TEST(test_pipe_fd);
    RUN_TEST(test_pipe_fd2);
    RUN_TEST(test_redirect);
    RUN_TEST(test_close_fd);
    RUN_TEST(test_readn_failure);
    RUN_TEST(test_readn_interrupted);
    RUN_TEST(test_writen_failure);
    RUN_TEST(test_writen_interrupted);
    RUN_TEST(test_pipe_fd_failure);
    RUN_TEST(test_pipe_fd2_failure);
    RUN_TEST(test_redirect_failure);
    RUN_TEST(test_close_fd_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}

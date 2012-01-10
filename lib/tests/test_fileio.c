/**
 * @file lib/tests/test_fileio.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-26 higashi 新規作成
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

#include <stdio.h>    /* tmpnam */
#include <unistd.h>   /* dup2 unlink pipe fork */
#include <fcntl.h>    /* open */
#include <sys/stat.h> /* chmod */
#include <sys/wait.h> /* wait */
#include <signal.h>   /* signal */
#include <errno.h>    /* errno */
#include <cutter.h>   /* cutter library */

#include "def.h"
#include "log.h"
#include "fileio.h"

#define BUF_SIZE 4100 /**< バッファサイズ */

/* プロトタイプ */
/** readn() 関数テスト */
void test_readn(void);
/** writen() 関数テスト */
void test_writen(void);
/** pipe_fd() 関数テスト */
void test_pipe_fd(void);
/** dup_fd() 関数テスト */
void test_dup_fd(void);
/** redirect() 関数テスト */
void test_redirect(void);
/** close_fd() 関数テスト */
void test_close_fd(void);

/* 内部変数 */
static char testfile[L_tmpnam] = {0}; /**< 一意なファイル名 */
static int fd = -1;                   /**< ファイルディスクリプタ */
static int pfd[] = { -1, -1 };        /**< パイプ */
static char sendbuf[BUF_SIZE];        /**< 送信バッファ */

/* 内部関数 */
/** 受信プロセス起動 */
static int read_child_process(char *readbuf, char *senddata, size_t len);
/** 送信プロセス起動 */
static int write_child_process(char *buf, size_t len);
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
    (void)memset(sendbuf, 'a', sizeof(sendbuf));
    sendbuf[sizeof(sendbuf) - 1] = '\0';
}

/**
 * 終了処理
 *
 * @return なし
 */
void
cut_teardown(void)
{
    int retval = 0; /* 戻り値 */

    if (fd != -1) {
        if (close(fd) < 0)
            cut_notify("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }
    if (pfd[PIPE_R] != -1) {
        if (close(pfd[PIPE_R]) < 0)
            cut_notify("close: fd=%d(%d)", pfd[PIPE_R], errno);
        pfd[PIPE_R] = -1;
    }
    if (pfd[PIPE_W] != -1) {
        if (close(pfd[PIPE_W]) < 0)
            cut_notify("close: fd=%d(%d)", pfd[PIPE_W], errno);
        pfd[PIPE_W] = -1;
    }

    if (testfile[0] != '\0') {
        if (!access(testfile, W_OK)) { /* ファイルが存在する */
            retval = chmod(testfile, S_IWUSR|S_IWGRP);
            if (retval < 0)
                cut_notify("chmod: %s(%d)", testfile, errno);
            retval = unlink(testfile);
            if (retval < 0)
                cut_notify("unlink: %s(%d)", testfile, errno);
        }
        (void)memset(testfile, 0, sizeof(testfile));
    }
}

/**
 * readn() 関数テスト
 *
 * @return なし
 */
void
test_readn(void)
{

    ssize_t rlen = 0;              /* 受信バイト数 */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    fd = read_child_process(readbuf, sendbuf, sizeof(sendbuf));
    if (fd < 0) {
        cut_error("write_child_process(%d)", errno);
        return;
    }

    dbglog("parent");
    rlen = writen(STDERR_FILENO, sendbuf, sizeof(sendbuf));
    if (rlen < 0) {
        cut_error("writen(%d)", errno);
        return;
    }

    w = wait(&status);
    if (w < 0)
        cut_notify("wait(%d)", errno);
    dbglog("w=%d", (int)w);
    cut_assert_equal_int(EXIT_SUCCESS, WEXITSTATUS(status));
}

/**
 * writen() 関数テスト
 *
 * @return なし
 */
void
test_writen(void)
{
    ssize_t rlen = 0;              /* 受信バイト数 */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    fd = write_child_process(sendbuf, sizeof(sendbuf));
    if (fd < 0) {
        cut_error("write_child_process(%d)", errno);
        return;
    }

    dbglog("parent");
    (void)memset(readbuf, 0, sizeof(readbuf));
    rlen = readn(fd, readbuf, sizeof(sendbuf));
    if (rlen < 0) {
        cut_error("readn(%d)", errno);
        return;
    }
    cut_assert_equal_string(sendbuf, readbuf);

    w = wait(&status);
    if (w < 0)
        cut_notify("wait(%d)", errno);
    dbglog("w=%d", (int)w);
    cut_assert_equal_int(EXIT_SUCCESS, WEXITSTATUS(status));
}

/**
 * pipe_fd() 関数テスト
 *
 * @return なし
 */
void
test_pipe_fd(void)
{
    ssize_t rlen = 0;              /* 受信バイト数 */
    ssize_t wlen = 0;              /* 送信バイト数 */
    pid_t cpid = 0;                /* 子プロセスID */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    cut_assert_operator(fd, >=, 0, cut_message("return value"));

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
    }

    if (cpid == 0) {
        dbglog("child");

        wlen = writen(STDERR_FILENO, sendbuf, sizeof(sendbuf));
        if (wlen < 0) {
            outlog("write: fd=%d", STDERR_FILENO);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        (void)memset(readbuf, 0, sizeof(readbuf));
        rlen = readn(fd, readbuf, sizeof(sendbuf));
        if (rlen < 0) {
            cut_error("read(%d)", errno);
            return;
        }
        cut_assert_equal_string(sendbuf, readbuf);

        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d(%d)", WEXITSTATUS(status), errno);
    }

    /* 異常系 */
    /* -1の場合 */
    fd = pipe_fd(-1);
    cut_assert_equal_int(EX_NG, fd, cut_message("return value"));
    /* 不正なファイルディスクリプタ */
    fd = pipe_fd(65535);
    cut_assert_equal_int(EX_NG, fd, cut_message("return value"));
}

/**
 * dup_fd() 関数テスト
 *
 * @return なし
 */
void
test_pipe_fd2(void)
{
    int retval = 0;                /* 戻り値 */
    ssize_t rlen = 0;              /* 受信バイト数 */
    ssize_t wlen = 0;              /* 送信バイト数 */
    pid_t cpid = 0;                /* 子プロセスID */
    pid_t w = 0;                   /* wait戻り値 */
    int status = 0;                /* ステータス */
    char readbuf[sizeof(sendbuf)]; /* 受信バッファ */
    int oldfd = 0;                 /* 退避用 */

    (void)memset(readbuf, 0, sizeof(readbuf));

    /* 正常系 */
    retval = pipe(pfd);
    if (retval < 0) {
        cut_error("pipe(%d)", errno);
        return;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
    }

    if (cpid == 0) {
        dbglog("child");

        oldfd = dup(STDIN_FILENO);
        if (oldfd < 0)
            cut_notify("dup(%d)", errno);
        retval = pipe_fd2(&pfd[PIPE_R], &pfd[PIPE_W], STDIN_FILENO);
        if (retval < 0) {
            outlog("dup_fd");
            exit(EXIT_FAILURE);
        }
        wlen = writen(STDIN_FILENO, sendbuf, sizeof(sendbuf));
        if (wlen < 0) {
            outlog("write");
            exit(EXIT_FAILURE);
        }
        dbglog("writen=%d, %s", wlen, sendbuf);

        if (dup2(oldfd, STDIN_FILENO) < 0)
            cut_notify("dup2(%d)", errno);

        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        oldfd = dup(STDIN_FILENO);
        if (oldfd < 0)
            cut_notify("dup(%d)", errno);
        retval = pipe_fd2(&pfd[PIPE_W], &pfd[PIPE_R], STDIN_FILENO);
        if (retval < 0) {
            cut_error("dup_fd(%d)", errno);
            return;
        }

        (void)memset(readbuf, 0, sizeof(readbuf));
        rlen = readn(STDIN_FILENO, readbuf, sizeof(sendbuf));
        if (rlen < 0) {
            cut_error("readn(%d)", errno);
            return;
        }
        dbglog("readn=%d, %s", rlen, readbuf);

        cut_assert_equal_int(EX_OK, retval, cut_message("return value"));
        cut_assert_equal_string(sendbuf, readbuf);

        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d(%d)", WEXITSTATUS(status), errno);
    }

    /* 異常系 */
    retval = pipe_fd2(&pfd[PIPE_W], &pfd[PIPE_R], STDIN_FILENO);
    cut_assert_equal_int(EX_NG, retval, cut_message("return value"));

    if (dup2(oldfd, STDIN_FILENO) < 0)
        cut_notify("dup2(%d)", errno);
}

/**
 * redirect() 関数テスト
 *
 * @return なし
 */
void
test_redirect(void)
{
    int retval = 0; /* 戻り値 */
    int oldfd = 0;  /* 退避用 */

    /* 正常系 */
    oldfd = dup(STDERR_FILENO);
    if (oldfd < 0)
        cut_notify("dup(%d)", errno);
    retval = redirect(STDERR_FILENO, "/dev/null");
    cut_assert_equal_int(EX_OK, retval, cut_message("redirect"));

    if (dup2(oldfd, STDERR_FILENO) < 0)
        cut_notify("dup2(%d)", errno);

    /* 異常系 */
    /* ファイルディスクリプタが-1 */
    retval = redirect(-1, "/dev/null");
    cut_assert_equal_int(EX_NG, retval,
                         cut_message("redirect: fd=-1"));

    /* ファイルパスがNULL */
    retval = redirect(STDERR_FILENO, NULL);
    cut_assert_equal_int(EX_NG, retval,
                         cut_message("redirect: path=null"));

    /* 書込権限なし */
    if (!tmpnam(testfile)) {
        cut_error("tmpnam(%d)", errno);
        return;
    }

    retval = creat(testfile, S_IRUSR|S_IRGRP);
    if (retval < 0) {
        cut_error("create: %s(%d)", testfile, errno);
        return;
    }

    retval = redirect(STDERR_FILENO, testfile);
    cut_assert_equal_int(EX_NG, retval,
                         cut_message("redirect: %s", testfile));
}

/**
 * close_fd() 関数テスト
 *
 * @return なし
 */
void
test_close_fd(void)
{
    int retval = 0;              /* 戻り値 */
    int fd[] = { -1, -1, -1 };   /* ファイルディスクリプタ */
    enum { FD1, FD2, FD3, MAX }; /* 配列要素 */

    /* 正常系 */
    int i;
    for (i = 0; i < MAX; i++) {
        fd[i] = open("/dev/null", O_WRONLY|O_APPEND);
        if (fd[i] < 0) {
            cut_error("open=%d(%d)", fd[i], errno);
            return;
        }
    }
    retval = close_fd(&fd[FD1], &fd[FD2], &fd[FD3], NULL);
    cut_assert_equal_int(-1, fd[FD1]);
    cut_assert_equal_int(-1, fd[FD2]);
    cut_assert_equal_int(-1, fd[FD3]);
    cut_assert_equal_int(EX_OK, retval, cut_message("retrun value"));

    /* 第二引数が-1の場合 */
    fd[FD1] = open("/dev/null", O_WRONLY|O_APPEND);
    if (fd[FD1] < 0) {
        cut_error("open=%d(%d)", fd[FD1], errno);
        return;
    }
    fd[FD3] = open("/dev/null", O_WRONLY|O_APPEND);
    if (fd[FD3] < 0) {
        cut_error("open=%d(%d)", fd[FD3], errno);
        return;
    }
    retval = close_fd(&fd[FD1], &fd[FD2], &fd[FD3], NULL);
    cut_assert_equal_int(-1, fd[FD1]);
    cut_assert_equal_int(-1, fd[FD3]);
    cut_assert_equal_int(EX_OK, retval, cut_message("retrun value"));

    /* 異常系 */
    fd[FD1] = 65535; /* オープンしていない */
    retval = close_fd(&fd[FD1], NULL);
    cut_assert_equal_int(-1, fd[FD1]);
    cut_assert_equal_int(EX_NG, retval, cut_message("retrun value"));
}

/**
 * 受信プロセス起動
 *
 * @return ファイルディスクリプタ
 * @retval EX_NG エラー
 */
static int
read_child_process(char *readbuf, char *senddata, size_t len)
{
    ssize_t rlen = 0; /* 受信バイト数 */
    pid_t cpid = 0;   /* 子プロセスID */
    int f = 0;        /* ファイルディスクリプタ */
    int retval = 0;   /* 戻り値 */

    f = pipe_fd(STDERR_FILENO);
    if (f < 0) {
        cut_error("pipe_fd: fd=%d", STDERR_FILENO);
        return EX_NG;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return EX_NG;
    }

    if (cpid == 0) {
        dbglog("child");

        rlen = readn(f, readbuf, len);
        if (rlen < 0) {
            outlog("readn: fds=%d", f);
            close_fd(&f, NULL);
            exit(EXIT_FAILURE);
        }
        dbglog("readn=%zd", rlen);
        retval = memcmp(senddata, readbuf, len);
        if (retval) { /* 非0 */
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
 * @return ファイルディスクリプタ
 * @retval EX_NG エラー
 */
static int
write_child_process(char *buf, size_t len)
{
    ssize_t wlen = 0; /* 送信バイト数 */
    pid_t cpid = 0;   /* 子プロセスID */
    int f = 0;        /* ファイルディスクリプタ */

    f = pipe_fd(STDERR_FILENO);
    if (f < 0) {
        cut_error("pipe_fd: fd=%d", STDERR_FILENO);
        return EX_NG;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return EX_NG;
    }

    if (cpid == 0) {
        dbglog("child");

        wlen = writen(STDERR_FILENO, buf, len);
        if (wlen < 0) {
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


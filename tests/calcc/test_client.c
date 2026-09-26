/**
 * @file tests/calcc/test_client.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-24 higashi 新規作成
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

#include <stdio.h>      /* setvbuf stdin stdout */
#include <string.h>     /* memset */
#include <unistd.h>     /* pipe fork */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* sockopt etc... */
#include <arpa/inet.h>  /* ntohl */
#include <sys/select.h> /* pselect */
#include <sys/wait.h>   /* wait */
#include <signal.h>     /* signal */
#include <errno.h>      /* errno */

#include "test_helper.h"
#include "test_process.h"

#include "def.h"
#include "log.h"
#include "net.h"
#include "data.h"
#include "fileio.h"
#include "memfree.h"
#include "client.h"

#define BUF_SIZE 4100 /**< バッファサイズ */

DEFINE_FFF_GLOBALS;

/* socket() と pselect() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(int, socket, int, int, int);
TEST_PASSTHROUGH(int, socket, (int domain, int type, int protocol),
                 (domain, type, protocol))
FAKE_VALUE_FUNC(int, pselect, int, fd_set *, fd_set *, fd_set *,
                const struct timespec *, const sigset_t *);
TEST_PASSTHROUGH(int, pselect,
                 (int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, const struct timespec *timeout,
                  const sigset_t *sigmask),
                 (nfds, readfds, writefds, exceptfds, timeout, sigmask))

/* 内部変数 */
static testclient client;                  /**< 関数構造体 */
static char port[] = "12345";              /**< ポート番号 */
static const char *hostname = "localhost"; /**< ホスト名 */
static struct sockaddr_in addr;            /**< ソケットアドレス情報構造体 */
static unsigned char readbuf[BUF_SIZE];    /**< 受信バッファ */
static unsigned char sendbuf[BUF_SIZE];    /**< 送信データ */
static int ssock = -1;                     /**< サーバソケット */
static int csock = -1;                     /**< クライアントソケット */
static int acc = -1;                       /**< アクセプト */
static int pfd1[] = { -1, -1 };            /**< パイプ1 */
static int pfd2[] = { -1, -1 };            /**< パイプ2 */
static const int CHILD_FAILED = 255;       /**< 子プロセス失敗 */

/* sigemptyset() などと, set_client_data() は, モックにして, 通常は本物を呼ぶ */
FAKE_VALUE_FUNC(int, sigemptyset, sigset_t *);
TEST_PASSTHROUGH(int, sigemptyset, (sigset_t *set), (set))
FAKE_VALUE_FUNC(int, sigfillset, sigset_t *);
TEST_PASSTHROUGH(int, sigfillset, (sigset_t *set), (set))
FAKE_VALUE_FUNC(int, sigdelset, sigset_t *, int);
TEST_PASSTHROUGH(int, sigdelset, (sigset_t *set, int signo), (set, signo))
FAKE_VALUE_FUNC(ssize_t, set_client_data, struct client_data **,
                const unsigned char *, size_t);
TEST_PASSTHROUGH(ssize_t, set_client_data,
                 (struct client_data **dt, const unsigned char *buf,
                  size_t len),
                 (dt, buf, len))

/*
 * atexit() は, glibc の静的ライブラリ (libc_nonshared.a) の, 小さな関数 (スタブ) で,
 * 共有ライブラリ (libcalcc.so) の中に取り込まれるので, 置き換えられない. スタブは,
 * __cxa_atexit() を呼ぶだけで, これは libc.so の関数なので, 置き換えられる.
 * 通常は, 本物の __cxa_atexit() を呼び, 注入したときだけ失敗させる.
 */
extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso);
static struct test_inject inject_cxa_atexit; /**< __cxa_atexit() に注入する失敗 */
static int (*real_cxa_atexit)(void (*)(void *), void *, void *) = NULL;
/**
 * __cxa_atexit() の置き換え (atexit() が呼ぶ)
 *
 * @param[in] func 終了時に呼ぶ関数
 * @param[in] arg 関数の引数
 * @param[in] dso 共有ライブラリのハンドル
 * @return 0, または注入した失敗
 */
int
__cxa_atexit(void (*func)(void *), void *arg, void *dso)
{
    if (inject_cxa_atexit.count > 0) {
        inject_cxa_atexit.count--;
        errno = inject_cxa_atexit.err;
        return (int)inject_cxa_atexit.value;
    }
    if (!real_cxa_atexit)
        *(void **)(&real_cxa_atexit) = dlsym(RTLD_NEXT, "__cxa_atexit");
    return real_cxa_atexit ? real_cxa_atexit(func, arg, dso) : -1;
}

/* プロトタイプ */
/** set_port_string() 関数テスト */
TEST test_set_port_string(void);
/** set_host_string() 関数テスト */
TEST test_set_host_string(void);
/** connect_sock() 関数テスト */
TEST test_connect_sock(void);
/** client_loop() 関数テスト */
TEST test_client_loop(void);
/** send_sock() 関数テスト */
TEST test_send_sock(void);
/** read_sock() 関数テスト */
TEST test_read_sock(void);
TEST test_connect_sock_failure(void);
TEST test_client_loop_failure(void);
TEST test_send_sock_failure(void);
TEST test_send_sock_timer(void);
TEST test_read_sock_failure(void);
TEST test_client_loop_signal_mask_failure(void);
TEST test_client_loop_atexit_failure(void);
TEST test_client_loop_socket_only(void);
TEST test_client_loop_read_failure(void);
TEST test_send_sock_alloc_failure(void);

/* 内部関数 */
static void on_sigint(int signo);
/** send_sock() 関数実行 */
static int exec_send_sock(unsigned char *sbuf, size_t length);
/** アクセプト */
static int accept_server(int sockfd);
/** 受信 */
static int recv_server(int sockfd, unsigned char *rbuf);
/** 送信 */
static int send_server(int sockfd, unsigned char *sbuf, size_t length);
/** ソケット生成 */
static int inet_sock_server(void);
/** シグナル設定 */
static void set_sig_handler(void);

/**
 * 初期化処理
 *
 * @return なし
 */
static void
startup(void)
{
    set_sig_handler();

    /* バッファリングしない */
    if (setvbuf(stdin, NULL, _IONBF, 0))
        TEST_NOTIFY("setvbuf: stdin(%d)", errno);
    if (setvbuf(stdout, NULL, _IONBF, 0))
        TEST_NOTIFY("setvbuf: stdout(%d)", errno);

    (void)memset(&client, 0, sizeof(testclient));
    test_init_client(&client);

    /* リダイレクト */
    redirect(STDERR_FILENO, "/dev/null");
}

/**
 * 初期化処理
 *
 * @return なし
 */
static void
setup(void *data)
{
    TEST_PASSTHROUGH_RESET(socket);
    TEST_PASSTHROUGH_RESET(pselect);
    (void)memset(&inject_cxa_atexit, 0, sizeof(inject_cxa_atexit));
    TEST_PASSTHROUGH_RESET(sigemptyset);
    TEST_PASSTHROUGH_RESET(sigfillset);
    TEST_PASSTHROUGH_RESET(sigdelset);
    TEST_PASSTHROUGH_RESET(set_client_data);
    FFF_RESET_HISTORY();
    g_gflag = false;
    g_tflag = false;
    g_sig_handled = 0;
    (void)memset(sendbuf, 'a', sizeof(sendbuf));
    sendbuf[sizeof(sendbuf) - 1] = '\0';
    sendbuf[sizeof(sendbuf) - 2] = '\n';

    (void)memset(readbuf, 0, sizeof(readbuf));
}

/**
 * 終了処理
 *
 * @return なし
 */
static void
teardown(void *data)
{
    int retval = 0; /* 戻り値 */

    close_fd(&pfd1[PIPE_R], &pfd1[PIPE_W],
             &pfd2[PIPE_R], &pfd2[PIPE_W], NULL);

    if (ssock != -1) {
        retval = shutdown(ssock, SHUT_RDWR);
        if (retval < 0)
            TEST_NOTIFY("shutdown=%d(%d)", ssock, errno);
    }

    close_sock(&acc);
    close_sock(&ssock);
    close_sock(&csock);
}

/**
 * test_set_port_string() 関数テスト
 *
 * @return なし
 */
TEST
test_set_port_string(void)
{
    int retval = 0;                  /* 戻り値 */
    const char *err_port = "123456"; /* エラー用 */

    /* 正常系 */
    retval = set_port_string(port);
    TEST_ASSERT_INT(EX_OK, retval);
    /* 異常系 */
    retval = set_port_string(err_port);
    TEST_ASSERT_INT(EX_NG, retval);
    PASS();
}

/**
 * test_set_host_string() 関数テスト
 *
 * @return なし
 */
TEST
test_set_host_string(void)
{
    int retval = 0;        /* 戻り値 */
    const char *err_host = /* エラー用 */
        "123456789012345678901234567890123456789012345678";

    /* 正常系 */
    retval = set_host_string(hostname);
    TEST_ASSERT_INT(EX_OK, retval);
    /* 異常系 */
    retval = set_host_string(err_host);
    TEST_ASSERT_INT(EX_NG, retval);
    PASS();
}

/**
 * test_connect_sock() 関数テスト
 *
 * @return なし
 */
TEST
test_connect_sock(void)
{
    dbglog("start");

    ssock = inet_sock_server();
    if (ssock < 0) {
        TEST_FAIL("inet_sock_server");
    }

    if (set_host_string(hostname) < 0)
        TEST_FAIL("set_host_string");
    if (set_port_string(port) < 0)
        TEST_FAIL("set_port_string");
    csock = connect_sock();
    dbglog("connect_sock=%d", csock);

    TEST_ASSERT_NOT_INT(EX_NG, csock);
    PASS();
}

/**
 * test_client_loop() 関数テスト
 *
 * @return なし
 */
TEST
test_client_loop(void)
{
    pid_t cpid = 0;            /* 子プロセスID */
    pid_t w = 0;               /* wait戻り値 */
    int status = 0;            /* wait引数 */
    ssize_t wlen = 0;          /* write戻り値 */
    ssize_t rlen = 0;          /* read戻り値 */
    int oldfd = 0;             /* 退避用 */
    size_t sendlen = 0;        /* 送信バイト数 */
    int retval = 0;            /* 戻り値 */
    st_client st = EX_SUCCESS; /* ステータス */

    dbglog("start");

    retval = pipe(pfd1);
    if (retval < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }

    retval = pipe(pfd2);
    if (retval < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }

    ssock = inet_sock_server();
    if (ssock < 0) {
        TEST_FAIL("inet_sock_server");
    }

    oldfd = dup(STDOUT_FILENO);
    if (oldfd < 0)
        TEST_NOTIFY("dup(%d)", errno);
    redirect(STDOUT_FILENO, "/dev/null");

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        /* コネクト */
        if (set_host_string(hostname) < 0 || set_port_string(port) < 0) {
            outlog("set_host_string/set_port_string");
            exit(CHILD_FAILED);
        }
        csock = connect_sock();
        if (csock < 0) {
            outlog("connect_sock");
            close_fd(&pfd1[PIPE_R], &pfd1[PIPE_W],
                     &pfd2[PIPE_R], &pfd2[PIPE_W], NULL);
            exit(CHILD_FAILED);
        }

        /* 標準入力 */
        retval = pipe_fd2(&pfd1[PIPE_W], &pfd1[PIPE_R], STDIN_FILENO);
        if (retval < 0) {
            outlog("pipe_fd2");
            close_fd(&pfd2[PIPE_R], &pfd2[PIPE_W], NULL);
            close_sock(&csock);
            exit(CHILD_FAILED);
        }

        /* 標準出力 */
        retval = pipe_fd2(&pfd2[PIPE_R], &pfd2[PIPE_W], STDOUT_FILENO);
        if (retval < 0) {
            outlog("pipe_fd2");
            close_sock(&csock);
            exit(CHILD_FAILED);
        }

        /* 送信と受信が終わるまでループを続け, 一定時間後に, SIGINT で終了する */
        (void)signal(SIGINT, on_sigint);
        if (fork() == 0) {
            (void)usleep(800000);
            (void)kill(getppid(), SIGINT);
            _exit(EXIT_SUCCESS);
        }
        st = client_loop(csock);

        close_sock(&csock);
        exit(st);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        /* 標準入力 */
        retval = pipe_fd2(&pfd1[PIPE_R], &pfd1[PIPE_W], STDIN_FILENO);
        if (retval < 0) {
            TEST_FAIL("pipe_fd2(%d)", errno);
        }

        /* 標準入力に送信 */
        sendlen = sizeof(sendbuf);
        /* 1 行だけ送る (終端の NUL まで送ると, 次のループで, 改行のない行を待ち続ける) */
        wlen = write(STDIN_FILENO, (char *)sendbuf, strlen((char *)sendbuf));
        if (wlen < 0) {
            TEST_FAIL("write=%zd(%d)", wlen, errno);
        }
        dbglog("write=%zd", wlen);

        /* 受信待ち */
        acc = accept_server(ssock);
        if (acc < 0) {
            TEST_FAIL("accept_server: acc=%d(%d)", acc, errno);
        }

        /* 受信 */
        retval = recv_server(acc, readbuf);
        if (retval < 0) {
            TEST_FAIL("recv_server: acc=%d(%d)", acc, errno);
        }

        /* 改行削除 */
        if (sendbuf[strlen((char *)sendbuf) - 1] == '\n')
            sendbuf[strlen((char *)sendbuf) - 1] = '\0';

        TEST_ASSERT_MEM(sendbuf, sendlen, readbuf, sendlen);

        /* 送信 */
        sendlen = strlen((char *)readbuf) + 1;
        retval = send_server(acc, readbuf, sendlen);
        if (retval < 0) {
            TEST_FAIL("send_server: acc=%d(%d)", acc, errno);
        }

        /* 標準出力 */
        retval = pipe_fd2(&pfd2[PIPE_W], &pfd2[PIPE_R], STDOUT_FILENO);
        if (retval < 0) {
            TEST_FAIL("pipe_fd2(%d)", errno);
        }

        /* 標準出力から受信 */
        (void)memset(readbuf, 0, sizeof(readbuf));
        rlen = readn(STDOUT_FILENO, (char *)readbuf, sendlen - 1);
        if (rlen < 0) {
            TEST_FAIL("read=%zd(%d)", rlen, errno);
        }
        dbglog("readbuf=%s", readbuf);

        /* 標準出力を元に戻す */
        if (dup2(oldfd, STDOUT_FILENO) < 0)
            TEST_NOTIFY("dup2(%d)", errno);

        TEST_ASSERT_MEM(sendbuf, sendlen, readbuf, sendlen);
        w = wait(&status);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) == CHILD_FAILED)
            TEST_FAIL("status=%d(%d)", WEXITSTATUS(status), errno);
    }
    PASS();
}

/**
 * test_send_sock() 関数テスト
 *
 * @return なし
 */
TEST
test_send_sock(void)
{
    int retval = 0;                  /* 戻り値 */
    unsigned char estr[] = "exit\n"; /* exit文字列 */
    unsigned char qstr[] = "quit\n"; /* quit文字列 */

    dbglog("start");

    retval = exec_send_sock(sendbuf, sizeof(sendbuf));
    TEST_ASSERT_INT(EX_SUCCESS, retval);
    teardown(NULL);

    retval = exec_send_sock(estr, sizeof(estr));
    TEST_ASSERT_INT(EX_QUIT, retval);
    teardown(NULL);

    retval = exec_send_sock(qstr, sizeof(qstr));
    TEST_ASSERT_INT(EX_QUIT, retval);
    teardown(NULL);
    PASS();
}

/**
 * test_read_sock() 関数テスト
 *
 * @return なし
 */
TEST
test_read_sock(void)
{
    pid_t cpid = 0;            /* プロセスID */
    pid_t w = 0;               /* wait戻り値 */
    int status = 0;            /* wait引数 */
    ssize_t rlen = 0;          /* read戻り値 */
    int oldfd = 0;             /* 退避用 */
    int retval = 0;            /* 戻り値 */
    st_client st = EX_SUCCESS; /* ステータス */

    dbglog("start");

    retval = pipe(pfd1);
    if (retval < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }

    ssock = inet_sock_server();
    if (ssock < 0) {
        TEST_FAIL("inet_sock_server");
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        if (set_host_string(hostname) < 0 || set_port_string(port) < 0) {
            outlog("set_host_string/set_port_string");
            exit(CHILD_FAILED);
        }
        csock = connect_sock();
        if (csock < 0) {
            outlog("connect_sock");
            close_fd(&pfd1[PIPE_R], &pfd1[PIPE_W], NULL);
            exit(CHILD_FAILED);
        }

        /* 標準出力 */
        if (pipe_fd2(&pfd1[PIPE_R], &pfd1[PIPE_W], STDOUT_FILENO) < 0) {
            outlog("pipe_fd2");
            close_sock(&csock);
            exit(CHILD_FAILED);
        }

        /* テスト関数 */
        st = client.read_sock(csock);

        close_sock(&csock);
        exit(st);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        /* 標準出力 */
        oldfd = dup(STDOUT_FILENO);
        retval = pipe_fd2(&pfd1[PIPE_W], &pfd1[PIPE_R], STDOUT_FILENO);
        if (retval < 0) {
            TEST_FAIL("pipe_fd2(%d)", errno);
        }

        /* 受信待ち */
        acc = accept_server(ssock);
        retval = send_server(acc, sendbuf, sizeof(sendbuf));
        if (retval < 0) {
            TEST_FAIL("send_server: acc=%d(%d)", acc, errno);
        }

        /* 標準出力から受信 */
        rlen = readn(STDOUT_FILENO, (char *)readbuf, sizeof(sendbuf));
        if (rlen < 0) {
            TEST_FAIL("read=%zd(%d)", rlen, errno);
        }

        /* 標準出力を元に戻す */
        if (dup2(oldfd, STDOUT_FILENO) < 0)
            TEST_NOTIFY("dup2(%d)", errno);

        TEST_ASSERT_MEM(sendbuf, strlen((char *)sendbuf), readbuf, strlen((char *)sendbuf));
        w = wait(&status);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            TEST_FAIL("status=%d(%d)", WEXITSTATUS(status), errno);
    }
    PASS();
}

/**
 * send_sock() 関数実行
 *
 * @return なし
 */
static int
exec_send_sock(unsigned char *sbuf, size_t length)
{
    pid_t cpid = 0;             /* プロセスID */
    pid_t w = 0;                /* wait戻り値 */
    int status = 0;             /* wait引数 */
    ssize_t wlen = 0;           /* write戻り値 */
    int oldfd = 0;              /* 退避用 */
    int retval = 0;             /* 戻り値 */
    unsigned char rbuf[length]; /* 受信バッファ */
    st_client st = EX_SUCCESS;  /* ステータス */

    dbglog("start");

    retval = pipe(pfd1);
    if (retval < 0) {
        TEST_ERROR("pipe(%d)", errno);
        return EX_NG;
    }

    ssock = inet_sock_server();
    if (ssock < 0) {
        TEST_ERROR("inet_sock_server");
        return EX_NG;
    }

    oldfd = dup(STDOUT_FILENO);
    if (oldfd < 0)
        TEST_NOTIFY("dup(%d)", errno);
    redirect(STDOUT_FILENO, "/dev/null");

    cpid = fork();
    if (cpid < 0) {
        TEST_ERROR("fork(%d)", errno);
        return EX_NG;
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        if (set_host_string(hostname) < 0 || set_port_string(port) < 0) {
            outlog("set_host_string/set_port_string");
            exit(CHILD_FAILED);
        }
        csock = connect_sock();
        if (csock < 0) {
            TEST_ERROR("pipe_fd2(%d)", errno);
            close_fd(&pfd1[PIPE_R], &pfd1[PIPE_W], NULL);
            exit(CHILD_FAILED);
        }

        /* 標準入力 */
        retval = pipe_fd2(&pfd1[PIPE_W], &pfd1[PIPE_R], STDIN_FILENO);
        if (retval < 0) {
            outlog("pipe_fd2");
            close_sock(&csock);
            exit(CHILD_FAILED);
        }

        /* テスト関数 */
        st = client.send_sock(csock);

        close_sock(&csock);
        exit(st);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        /* 標準入力 */
        retval = pipe_fd2(&pfd1[PIPE_R], &pfd1[PIPE_W], STDIN_FILENO);
        if (retval < 0) {
            TEST_ERROR("pipe_fd2(%d)", errno);
            return EX_NG;
        }

        /* 標準入力に送信 */
        wlen = writen(STDIN_FILENO, (char *)sbuf, length);
        if (wlen < 0) {
            TEST_ERROR("write=%zd(%d)", wlen, errno);
            return EX_NG;
        }
        dbglog("write=%zd", wlen);

        if (strcmp((char *)sbuf, "quit\n") &&
            strcmp((char *)sbuf, "exit\n")) {

            /* 受信待ち */
            acc = accept_server(ssock);
            if (acc < 0) {
                TEST_ERROR("accept(%d)", errno);
                return EX_NG;
            }

            /* 受信 */
            retval = recv_server(acc, rbuf);
            if (retval < 0) {
                TEST_ERROR("recv_server: acc=%d(%d)", acc, errno);
                return EX_NG;
            }

            /* 改行削除 */
            if (sbuf[strlen((char *)sbuf) - 1] == '\n')
                sbuf[strlen((char *)sbuf) - 1] = '\0';

            if (strcmp((char *)sbuf, (char *)rbuf)) {
                TEST_ERROR("expected=%s actual=%s", sbuf, rbuf);
                return EX_NG;
            }
        }

        /* 標準出力を元に戻す */
        if (dup2(oldfd, STDOUT_FILENO) < 0)
            TEST_NOTIFY("dup2(%d)", errno);

        w = wait(&status);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) == CHILD_FAILED)
            TEST_ERROR("child failed");
    }
    return WEXITSTATUS(status);
}

/**
 * アクセプト
 *
 * @param[in] sockfd ソケット
 * @return アクセプト
 */
static int
accept_server(int sockfd)
{
    int ready = 0;           /* pselect戻り値 */
    int accfd = -1;          /* アクセプト */
    socklen_t addrlen = 0;   /* addr構造体の長さ */
    fd_set fds, rfds;        /* selectマスク */
    struct timespec timeout; /* タイムアウト値 */
    sigset_t sigmask;        /* シグナルマスク */

    dbglog("start: sockfd=%d", sockfd);

    /* マスクの設定 */
    FD_ZERO(&fds);        /* 初期化 */
    FD_SET(sockfd, &fds); /* ソケットをマスク */

    /* シグナルマスクの設定 */
    if (sigemptyset(&sigmask) < 0) /* 初期化 */
        outlog("sigemptyset=0x%x", sigmask);
    if (sigfillset(&sigmask) < 0)  /* シグナル全て */
        outlog("sigfillset=0x%x", sigmask);

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timespec));
    /* pselectの場合, constなのでループ前で値を入れる */
    timeout.tv_sec = 5; /* 5秒 */
    timeout.tv_nsec = 0;

    while (true) {
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        ready = pselect(sockfd + 1, &rfds,
                        NULL, NULL, &timeout, &sigmask);
        if (ready < 0) {
            if (errno == EINTR) /* 割り込み */
                continue;
            TEST_NOTIFY("select=%d", ready);
            break;
        } else if (ready) {
            if (FD_ISSET(sockfd, &rfds)) {
                /* アクセプト */
                addrlen = (socklen_t)sizeof(struct sockaddr_in);
                accfd = accept(sockfd, (struct sockaddr *)&addr, &addrlen);
                if (accfd < 0) {
                    TEST_NOTIFY("accept=%d(%d)", accfd, errno);
                    return EX_NG;
                }
                dbglog("accept=%d(%d)", accfd, errno);
                break;
            }
        } else { /* タイムアウト */
            break;
        }
    }
    return accfd;
}

/**
 * 受信
 *
 * @param[in] sockfd ソケット
 * @param[in] rbuf 受信バッファ
 * @retval EX_NG エラー
 */
static int
recv_server(int sockfd, unsigned char *rbuf)
{
    size_t length = 0; /* バイト数 */
    struct header hd;  /* ヘッダ構造体 */
    int retval = 0;    /* 戻り値 */

    dbglog("start");

    /* ヘッダ受信 */
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);

    retval = recv_data(sockfd, &hd, &length);
    if (retval < 0) {
        TEST_NOTIFY("recv_data: length=%zu(%d)", length, errno);
        return EX_NG;
    }
    length = (size_t)ntohl((uint32_t)hd.length);

    /* 受信 */
    retval = recv_data(sockfd, rbuf, &length);
    if (retval < 0) {
        TEST_NOTIFY("recv_data: length=%zu(%d)", length, errno);
        return EX_NG;
    }
    return EX_OK;
}

/**
 * 送信
 *
 * @param[in] sockfd ソケット
 * @param[in] sbuf 送信バッファ
 * @param[in] length バイト数
 * @retval EX_NG エラー
 */
static int
send_server(int sockfd, unsigned char *sbuf, size_t length)
{
    struct server_data *sdata = NULL; /* 送信データ構造体 */
    ssize_t slen = 0;                 /* 送信データバイト数 */
    int retval = 0;                   /* 戻り値 */

    dbglog("start");

    /* データ設定 */
    slen = set_server_data(&sdata, sbuf, length);
    if (slen < 0) {
        TEST_NOTIFY("set_server_data=%zd(%d)", slen, errno);
        return EX_NG;
    }

    /* 送信 */
    retval = send_data(sockfd, sdata, (size_t *)&slen);
    if (retval < 0) {
        TEST_NOTIFY("send_data: slen=%zd(%d)", slen, errno);
        memfree((void **)&sdata, NULL);
        return EX_NG;
    }
    memfree((void **)&sdata, NULL);
    return EX_OK;
}

/**
 * ソケット生成
 *
 * @return ソケット
 */
static int
inet_sock_server(void)
{
    int retval = 0; /* 戻り値 */
    int sockfd = 0; /* ソケット */

    dbglog("start");

    (void)memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ポート番号を設定 */
    if (set_port(&addr, port) < 0)
        return EX_NG;

    /* ソケット生成 */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        TEST_NOTIFY("socket(%d)", errno);
        return EX_NG;
    }

    /* ソケットオプション */
    int optval = 1; /* 二値オプション有効 */
    retval = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                        &optval, (socklen_t)sizeof(int));
    if (retval < 0) {
        TEST_NOTIFY("setsockopt: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    /* ソケットにアドレスを指定 */
    retval = bind(sockfd, (struct sockaddr *)&addr,
                  (socklen_t)sizeof(addr));
    if (retval < 0) {
        if (errno == EADDRINUSE)
            TEST_NOTIFY("Address already in use\n");
        TEST_NOTIFY("bind: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    /* アクセスバックログの指定 */
    retval = listen(sockfd, SOMAXCONN);
    if (retval < 0) {
        TEST_NOTIFY("listen: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    /* ノンブロッキングモードに設定 */
    retval = set_block(sockfd, NONBLOCK);
    if (retval < 0) {
        TEST_NOTIFY("set_block: sockfd=%d", sockfd);
        return EX_NG;
    }

    return sockfd;
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
        TEST_NOTIFY("SIGINT");
    if (signal(SIGTERM, SIG_IGN) < 0)
        TEST_NOTIFY("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) < 0)
        TEST_NOTIFY("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) < 0)
        TEST_NOTIFY("SIGHUP");
    if (signal(SIGALRM, SIG_IGN) < 0)
        TEST_NOTIFY("SIGALRM");
}


/* 子プロセスで実行する処理の種類 */
static int child_mode = 0;   /**< 処理の種類 */
static int child_sock = -1;  /**< 使用するソケット */

/**
 * client_loop() を子プロセスで実行する
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_client_loop(void *arg)
{
    (void)arg;
    (void)signal(SIGPIPE, SIG_IGN);
    exit(client_loop(child_sock));
}

/**
 * send_sock() を子プロセスで実行する
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_send_sock(void *arg)
{
    (void)arg;
    (void)signal(SIGPIPE, SIG_IGN);
    exit(client.send_sock(child_sock));
}

/**
 * read_sock() を子プロセスで実行する
 * 接続先から, child_mode に応じたデータを送って, 受信する.
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_read_sock(void *arg)
{
    int sv[2] = { -1, -1 };        /* ソケットペア */
    struct header hd;              /* ヘッダ */
    struct server_data *dt = NULL; /* 送信データ */
    ssize_t len = 0;               /* 送信データ長 */

    (void)arg;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
        exit(CHILD_FAILED);

    (void)memset(&hd, 0, sizeof(hd));
    switch (child_mode) {
    case 0: /* 何も送らずに閉じる: ヘッダを受信できない */
        break;
    case 1: /* データ長が 4 のヘッダだけ送って閉じる: データを受信できない */
        hd.length = htonl(4);
        (void)writen(sv[1], &hd, sizeof(hd));
        break;
    case 2: /* データ長が 0 のヘッダ */
        hd.length = htonl(0);
        (void)writen(sv[1], &hd, sizeof(hd));
        break;
    default: /* 正常なデータ (4 は, 標準出力を閉じて, 出力に失敗する) */
        len = set_server_data(&dt, (const unsigned char *)"42", 3);
        if (len < 0)
            exit(CHILD_FAILED);
        (void)writen(sv[1], dt, (size_t)len);
        break;
    }
    (void)close(sv[1]);
    if (child_mode == 4)
        (void)close(STDOUT_FILENO);
    exit(client.read_sock(sv[0]));
}

/**
 * connect_sock() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_connect_sock_failure(void)
{
    /* ホスト名を解決できない */
    TEST_ASSERT_INT(EX_OK, set_host_string(""));
    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    TEST_ASSERT_INT(EX_NG, connect_sock());

    /* ポート番号を解決できない (存在しないサービス名. 5文字まで) */
    TEST_ASSERT_INT(EX_OK, set_host_string(hostname));
    TEST_ASSERT_INT(EX_OK, set_port_string("nosvc"));
    TEST_ASSERT_INT(EX_NG, connect_sock());

    /* socket() に失敗 */
    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    TEST_INJECT(socket, 0, 1, -1, EMFILE);
    TEST_ASSERT_INT(EX_NG, connect_sock());
    TEST_ASSERT_INJECTED(socket);

    /* connect() に失敗 (接続先がない) */
    csock = connect_sock();
    TEST_ASSERT_INT(EX_NG, csock);
    PASS();
}

/**
 * client_loop() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_client_loop_failure(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */
    int sv[2] = { -1, -1 };   /* ソケットペア */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }
    child_sock = sv[0];

    /* pselect() が割り込まれた */
    TEST_INJECT(pselect, 0, 1, -1, EINTR);
    TEST_ASSERT_INT(EX_SIGNAL,
                    test_run_child(child_client_loop, NULL, NULL, out,
                                   sizeof(out)));

    /* pselect() に失敗 */
    TEST_INJECT(pselect, 0, 1, -1, EBADF);
    TEST_ASSERT_INT(EX_FAILURE,
                    test_run_child(child_client_loop, NULL, NULL, out,
                                   sizeof(out)));

    /* タイムアウトして, シグナルを受け取っていれば, ループを終了する */
    g_sig_handled = 1;
    TEST_INJECT(pselect, 0, 1, 0, 0);
    TEST_ASSERT_INT(EX_SIGNAL,
                    test_run_child(child_client_loop, NULL, NULL, out,
                                   sizeof(out)));

    /* 標準入力が空行なら, 次のループへ (シグナルを受け取っていれば終了) */
    TEST_INJECT(pselect, 0, 1, 1, 0);
    TEST_ASSERT_INT(EX_SIGNAL,
                    test_run_child(child_client_loop, NULL, "\n", out,
                                   sizeof(out)));
    g_sig_handled = 0;

    /* 標準入力が quit なら, 終了 */
    TEST_INJECT(pselect, 0, 1, 1, 0);
    TEST_ASSERT_INT(EX_QUIT,
                    test_run_child(child_client_loop, NULL, "quit\n", out,
                                   sizeof(out)));

    /* 受信に失敗 (接続先が閉じている) */
    (void)close(sv[1]);
    TEST_INJECT(pselect, 0, 1, 1, 0);
    TEST_ASSERT_INT(EX_SEND_ERR,
                    test_run_child(child_client_loop, NULL, "1+1\n", out,
                                   sizeof(out)));
    (void)close(sv[0]);
    PASS();
}

/**
 * send_sock() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_send_sock_failure(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    child_sock = -1;

    /* 標準入力が閉じている */
    TEST_ASSERT_INT(EX_ALLOC_ERR,
                    test_run_child(child_send_sock, NULL, NULL, out,
                                   sizeof(out)));
    /* 空行 */
    TEST_ASSERT_INT(EX_EMPTY,
                    test_run_child(child_send_sock, NULL, "\n", out,
                                   sizeof(out)));
    /* 不正なソケットには, 送信できない */
    TEST_ASSERT_INT(EX_SEND_ERR,
                    test_run_child(child_send_sock, NULL, "1+1\n", out,
                                   sizeof(out)));
    PASS();
}

/**
 * send_sock() と read_sock() 関数テスト (処理時間の表示とデバッグ出力)
 *
 * @return なし
 */
TEST
test_send_sock_timer(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */
    int sv[2] = { -1, -1 };   /* ソケットペア */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }

    /* 送信 */
    g_tflag = true;
    g_gflag = true;
    child_sock = sv[0];
    TEST_ASSERT_INT(EX_SUCCESS,
                    test_run_child(child_send_sock, NULL, "1+1\n", out,
                                   sizeof(out)));
    (void)close(sv[0]);
    (void)close(sv[1]);

    /* 受信 (処理時間も表示する) */
    child_mode = 3;
    TEST_ASSERT_INT(EX_SUCCESS,
                    test_run_child(child_read_sock, NULL, NULL, out,
                                   sizeof(out)));
    TEST_ASSERT_MATCH("time of client_time: [0-9]+\\.[0-9]+\\[msec\\]", out);
    TEST_ASSERT_MATCH("42", out);
    PASS();
}

/**
 * read_sock() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_read_sock_failure(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* ヘッダを受信できない */
    child_mode = 0;
    TEST_ASSERT_INT(EX_RECV_ERR,
                    test_run_child(child_read_sock, NULL, NULL, out,
                                   sizeof(out)));
    /* データを受信できない */
    child_mode = 1;
    TEST_ASSERT_INT(EX_ALLOC_ERR,
                    test_run_child(child_read_sock, NULL, NULL, out,
                                   sizeof(out)));
    /* データ長が 0 */
    child_mode = 2;
    TEST_ASSERT_INT(EX_RECV_ERR,
                    test_run_child(child_read_sock, NULL, NULL, out,
                                   sizeof(out)));
    /* 標準出力に書き込めなくても, 続行する */
    child_mode = 4;
    TEST_ASSERT_INT(EX_SUCCESS,
                    test_run_child(child_read_sock, NULL, NULL, out,
                                   sizeof(out)));
    PASS();
}

/**
 * シグナルマスクの取得に失敗する client_loop() を, 子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_client_loop_mask_failure(void *arg)
{
    (void)arg;
    TEST_INJECT(sigemptyset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigfillset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigdelset, 0, 1, -1, EINVAL);
    TEST_INJECT(pselect, 0, 1, -1, EINTR);
    exit(client_loop(child_sock));
}

/**
 * 送信のあとの受信に失敗する client_loop() を, 子プロセスで実行するための関数
 * 接続先は, ヘッダだけ送って, 書込側をシャットダウンする.
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_client_loop_read_failure(void *arg)
{
    int sv[2] = { -1, -1 }; /* ソケットペア */
    struct header hd;       /* ヘッダ */

    (void)arg;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
        exit(CHILD_FAILED);
    (void)memset(&hd, 0, sizeof(hd));
    hd.length = htonl(4);
    (void)writen(sv[1], &hd, sizeof(hd));
    (void)shutdown(sv[1], SHUT_WR); /* 読込側は開いているので, 送信は成功する */

    TEST_INJECT(pselect, 0, 1, 1, 0);
    exit(client_loop(sv[0]));
}

/**
 * client_loop() 関数テスト (シグナルマスクの取得に失敗)
 *
 * @return なし
 */
TEST
test_client_loop_signal_mask_failure(void)
{
    child_sock = -1;
    /* 失敗しても, 続行する */
    TEST_ASSERT_INT(EX_SIGNAL,
                    test_run_child(child_client_loop_mask_failure, NULL, NULL,
                                   NULL, 0));
    PASS();
}

/**
 * client_loop() 関数テスト (受信に失敗すると, そのステータスで終了)
 *
 * @return なし
 */
TEST
test_client_loop_read_failure(void)
{
    /* 送信のあとの受信で, データを受信できない (EX_ALLOC_ERR) */
    TEST_ASSERT_INT(EX_ALLOC_ERR,
                    test_run_child(child_client_loop_read_failure, NULL,
                                   "1+1\n", NULL, 0));
    PASS();
}

/**
 * send_sock() 関数テスト (送信データを作れない)
 *
 * @return なし
 */
TEST
test_send_sock_alloc_failure(void)
{
    child_sock = -1;
    TEST_INJECT(set_client_data, 0, 1, EX_NG, ENOMEM);
    TEST_ASSERT_INT(EX_ALLOC_ERR,
                    test_run_child(child_send_sock, NULL, "1+1\n", NULL, 0));
    PASS();
}

/**
 * atexit() に失敗する client_loop() を, 子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_client_loop_atexit_failure(void *arg)
{
    (void)arg;
    inject_cxa_atexit.count = 1;
    inject_cxa_atexit.value = -1;
    inject_cxa_atexit.err = ENOMEM;
    exit(client_loop(child_sock));
}

/**
 * client_loop() 関数テスト (atexit() の失敗)
 *
 * @return なし
 */
TEST
test_client_loop_atexit_failure(void)
{
    child_sock = -1;
    TEST_ASSERT_INT(EX_FAILURE,
                    test_run_child(child_client_loop_atexit_failure, NULL, NULL,
                                   NULL, 0));
    PASS();
}

/**
 * SIGINT のハンドラ (client/main.c と同じ動作)
 *
 * @param[in] signo シグナル
 * @return なし
 */
static void
on_sigint(int signo)
{
    (void)signo;
    g_sig_handled = 1;
}

/**
 * ソケットだけが読める状態の client_loop() を, 子プロセスで実行するための関数
 * 標準入力は, 何も入力されない (書込側を開いたままの) パイプにする.
 * 接続先は, 答えを送っておく. 一定時間後に, SIGINT で, ループを終了する.
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
child_client_loop_socket_only(void *arg)
{
    int idle[2] = { -1, -1 };      /* 標準入力用のパイプ */
    int sv[2] = { -1, -1 };        /* ソケットペア */
    struct server_data *dt = NULL; /* 送信データ */
    ssize_t len = 0;               /* 送信データ長 */
    pid_t ppid = getpid();         /* client_loop() を実行するプロセス */

    (void)arg;
    (void)signal(SIGALRM, SIG_DFL); /* startup() で無視している */
    (void)alarm(10);                /* 終了しなかったときの保険 */
    if (pipe(idle) < 0 || socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
        exit(CHILD_FAILED);
    (void)dup2(idle[0], STDIN_FILENO);
    len = set_server_data(&dt, (const unsigned char *)"42", 3);
    if (len < 0 || writen(sv[1], dt, (size_t)len) < 0)
        exit(CHILD_FAILED);

    (void)signal(SIGINT, on_sigint);
    if (fork() == 0) {
        (void)usleep(500000);
        (void)kill(ppid, SIGINT);
        _exit(EXIT_SUCCESS);
    }
    exit(client_loop(sv[0]));
}

/**
 * client_loop() 関数テスト (ソケットだけが読める)
 * 標準入力が読めない間は, 標準入力を待たずに, ソケットから受信する.
 *
 * @return なし
 */
TEST
test_client_loop_socket_only(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    TEST_ASSERT_INT(EX_SIGNAL,
                    test_run_child(child_client_loop_socket_only, NULL, NULL,
                                   out, sizeof(out)));
    TEST_ASSERT_STR("42\n", out);
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    startup();
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(test_set_port_string);
    RUN_TEST(test_set_host_string);
    RUN_TEST(test_connect_sock);
    RUN_TEST(test_client_loop);
    RUN_TEST(test_send_sock);
    RUN_TEST(test_read_sock);
    RUN_TEST(test_connect_sock_failure);
    RUN_TEST(test_client_loop_failure);
    RUN_TEST(test_send_sock_failure);
    RUN_TEST(test_send_sock_timer);
    RUN_TEST(test_read_sock_failure);
    RUN_TEST(test_client_loop_signal_mask_failure);
    RUN_TEST(test_client_loop_read_failure);
    RUN_TEST(test_client_loop_atexit_failure);
    RUN_TEST(test_client_loop_socket_only);
    RUN_TEST(test_send_sock_alloc_failure);
    TEST_MAIN_END();
}

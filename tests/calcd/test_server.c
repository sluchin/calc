/**
 * @file tests/calcd/test_server.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-24 higashi 新規作成
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

#include <stdio.h>      /* setvbuf stdin stdout */
#include <string.h>     /* memset */
#include <unistd.h>     /* pipe fork */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* sockopt etc... */
#include <arpa/inet.h>  /* ntohl */
#include <sys/wait.h>   /* wait */
#include <sys/select.h> /* pselect */
#include <pthread.h>    /* pthread_create pthread_detach */
#include <signal.h>     /* signal */
#include <errno.h>      /* errno */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "net.h"
#include "data.h"
#include "fileio.h"
#include "memfree.h"
#include "server.h"
#include "calc.h"

#define BUF_SIZE 30u /**< バッファサイズ */

DEFINE_FFF_GLOBALS

/* システムコールなどは, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(int, socket, int, int, int)
TEST_PASSTHROUGH(int, socket, (int domain, int type, int protocol), (domain, type, protocol))
FAKE_VALUE_FUNC(int, setsockopt, int, int, int, const void *, socklen_t)
TEST_PASSTHROUGH(int,
                 setsockopt,
                 (int fd, int level, int name, const void *val, socklen_t len),
                 (fd, level, name, val, len))
FAKE_VALUE_FUNC(int, listen, int, int)
TEST_PASSTHROUGH(int, listen, (int fd, int backlog), (fd, backlog))
FAKE_VALUE_FUNC(
    int, pselect, int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *)
TEST_PASSTHROUGH(int,
                 pselect,
                 (int nfds,
                  fd_set *readfds,
                  fd_set *writefds,
                  fd_set *exceptfds,
                  const struct timespec *timeout,
                  const sigset_t *sigmask),
                 (nfds, readfds, writefds, exceptfds, timeout, sigmask))
/* glibc の accept() の引数は, 透過的共用体 (GNU 拡張) なので, ISO C では型が一致しない */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
FAKE_VALUE_FUNC(int, accept, int, struct sockaddr *, socklen_t *)
TEST_PASSTHROUGH(int, accept, (int fd, struct sockaddr *addr, socklen_t *len), (fd, addr, len))
#pragma GCC diagnostic pop
/* FFF は, 関数ポインタの型を直接書けないので, typedef する */
/** スレッド関数の型 */
typedef void *(*thread_func_t)(void *);
FAKE_VALUE_FUNC(int, pthread_create, pthread_t *, const pthread_attr_t *, thread_func_t, void *)
TEST_PASSTHROUGH(int,
                 pthread_create,
                 (pthread_t * tid, const pthread_attr_t *attr, void *(*func)(void *), void *arg),
                 (tid, attr, func, arg))
FAKE_VALUE_FUNC(int, pthread_detach, pthread_t)
TEST_PASSTHROUGH(int, pthread_detach, (pthread_t tid), (tid))

/* 計算やデータ作成, 送信と, シグナルマスクの関数は, モックにして, 通常は本物を呼ぶ */
FAKE_VALUE_FUNC(unsigned char *, create_answer, calcinfo *, const unsigned char *)
TEST_PASSTHROUGH(unsigned char *,
                 create_answer,
                 (calcinfo * calc, const unsigned char *expr),
                 (calc, expr))
FAKE_VALUE_FUNC(ssize_t, set_server_data, struct server_data **, const unsigned char *, size_t)
TEST_PASSTHROUGH(ssize_t,
                 set_server_data,
                 (struct server_data * *dt, const unsigned char *buf, size_t len),
                 (dt, buf, len))
FAKE_VALUE_FUNC(int, send_data, int, const void *, size_t *)
TEST_PASSTHROUGH(int,
                 send_data,
                 (const int sock, const void *data, size_t *length),
                 (sock, data, length))
FAKE_VALUE_FUNC(int, sigemptyset, sigset_t *)
TEST_PASSTHROUGH(int, sigemptyset, (sigset_t * set), (set))
FAKE_VALUE_FUNC(int, sigfillset, sigset_t *)
TEST_PASSTHROUGH(int, sigfillset, (sigset_t * set), (set))
FAKE_VALUE_FUNC(int, sigdelset, sigset_t *, int)
TEST_PASSTHROUGH(int, sigdelset, (sigset_t * set, int signo), (set, signo))
FAKE_VALUE_FUNC(int, pthread_sigmask, int, const sigset_t *, sigset_t *)
TEST_PASSTHROUGH(int,
                 pthread_sigmask,
                 (int how, const sigset_t *set, sigset_t *oldset),
                 (how, set, oldset))

/*
 * malloc() は, ほとんどの関数が使うので, FFF のモックにはせず, 指定したサイズの
 * ときだけ失敗させる (FFF のモックは, main() より前の呼び出しでも使われる).
 * 本物は __libc_malloc() で呼ぶ.
 */
extern void *__libc_malloc(size_t size);               /**< 本物の malloc() */
extern void *__libc_calloc(size_t nmemb, size_t size); /**< 本物の calloc() */
extern void __libc_free(void *ptr);                    /**< 本物の free() */
static size_t fail_malloc_size = 0u;      /**< 失敗させる malloc() のサイズ (0 は無効) */
static int fail_malloc_count = 0;         /**< 失敗させる回数 */
static size_t track_size = 0u;            /**< 確保と解放を追跡するサイズ (0 は無効) */
static void *volatile tracked_ptr = NULL; /**< 追跡している, 確保したメモリ */
static volatile int tracked_freed = 0;    /**< 追跡しているメモリが解放された */
static size_t watch_size = 0u;            /**< 確保されたかを数えるサイズ (0 は無効) */
static volatile int watch_count = 0;      /**< watch_size で確保された回数 */

/**
 * 確保したメモリの記録 (malloc() と calloc() の共通処理)
 *
 * @param[in] ptr 確保したメモリ
 * @param[in] size サイズ
 */
static void
record_alloc(void *ptr, size_t size)
{
    if ((track_size != 0u) && (size == track_size))
        tracked_ptr = ptr;
    if ((watch_size != 0u) && (size == watch_size))
        watch_count++;
}

/**
 * malloc() の置き換え
 *
 * @param[in] size サイズ
 * @return 確保したメモリ. 指定したサイズのときは, 失敗 (NULL)
 */
void *
malloc(size_t size)
{
    void *ptr = NULL; /* 確保したメモリ */

    if ((fail_malloc_count > 0) && (size == fail_malloc_size)) {
        fail_malloc_count--;
        errno = ENOMEM;
        return NULL;
    }
    ptr = __libc_malloc(size);
    record_alloc(ptr, size);
    return ptr;
}

/*
 * malloc() のあとに memset(0) するコードは, 最適化で calloc() になるので, calloc() も
 * 同じように置き換える.
 */
/**
 * calloc() の置き換え
 *
 * @param[in] nmemb 要素数
 * @param[in] size 要素のサイズ
 * @return 確保したメモリ. 指定したサイズのときは, 失敗 (NULL)
 */
void *
calloc(size_t nmemb, size_t size)
{
    void *ptr = NULL; /* 確保したメモリ */

    if ((fail_malloc_count > 0) && (nmemb * size == fail_malloc_size)) {
        fail_malloc_count--;
        errno = ENOMEM;
        return NULL;
    }
    ptr = __libc_calloc(nmemb, size);
    record_alloc(ptr, nmemb * size);
    return ptr;
}

/**
 * free() の置き換え (追跡しているメモリが解放されたか記録する)
 *
 * @param[in] ptr 解放するメモリ
 */
void
free(void *ptr)
{
    if ((ptr != NULL) && (ptr == tracked_ptr))
        tracked_freed = 1;
    __libc_free(ptr);
}

#define THREAD_WAIT 200000 /**< スレッドの終了を待つ時間 (マイクロ秒) */

/** スレッドデータ構造体 */
struct send_data {
    unsigned char sdata[BUF_SIZE];    /**< 送信データ */
    unsigned char rdata[BUF_SIZE];    /**< 受信データ */
    unsigned char expected[BUF_SIZE]; /**< 期待される文字列 */
    size_t len;                       /**< 送信データ長 */
};

/* プロトタイプ */
TEST test_set_port_string(void);
TEST test_server_sock(void);
TEST test_server_loop(void);
TEST test_server_sock_failure(void);
TEST test_server_loop_failure(void);
TEST test_server_proc_failure(void);
TEST test_server_loop_alloc_failure(void);
TEST test_server_loop_signal_mask_failure(void);
TEST test_server_proc_internal_failure(void);
TEST test_server_proc_free_arg(void);
TEST test_server_proc_length_limit(void);
TEST test_server_proc_no_nul(void);

/* 内部変数 */
static testserver server;                  /**< 関数構造体 */
static char port[] = "12345";              /**< ポート番号 */
static const char *hostname = "localhost"; /**< ホスト名 */
static unsigned char readbuf[BUF_SIZE];    /**< 受信バッファ */
static unsigned char expr[] = "1+1";       /**< 式 */
static unsigned char expected[] = "2";     /**< 期待される文字列 */
static int ssock = -1;                     /**< サーバソケット */
static int csock = -1;                     /**< クライアントソケット */

/* 内部関数 */
static int send_client(int sockfd, unsigned char *sbuf, size_t length);
static int recv_client(int sockfd, unsigned char *rbuf);
static int inet_sock_client(void);
static void set_sig_handler(void);

/**
 * 初期化処理
 */
static void
startup(void)
{
    (void)signal(SIGPIPE, SIG_IGN);
    set_sig_handler();

    /* バッファリングしない */
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0)
        TEST_NOTIFY("setvbuf: stdin(%d)", errno);
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
        TEST_NOTIFY("setvbuf: stdout(%d)", errno);

    (void)memset(&server, 0, sizeof(testserver));
    test_init_server(&server);

    /* リダイレクト */
    redirect(STDERR_FILENO, "/dev/null");
}

/**
 * 初期化処理
 *
 * @param[in] data 使用しない
 */
static void
setup(void *data)
{
    (void)data; /* 使用しない */
    /* モックと状態を, 初期状態 (素通し) に戻す */
    TEST_PASSTHROUGH_RESET(socket);
    TEST_PASSTHROUGH_RESET(setsockopt);
    TEST_PASSTHROUGH_RESET(listen);
    TEST_PASSTHROUGH_RESET(pselect);
    TEST_PASSTHROUGH_RESET(accept);
    TEST_PASSTHROUGH_RESET(pthread_create);
    TEST_PASSTHROUGH_RESET(pthread_detach);
    TEST_PASSTHROUGH_RESET(create_answer);
    TEST_PASSTHROUGH_RESET(set_server_data);
    TEST_PASSTHROUGH_RESET(send_data);
    TEST_PASSTHROUGH_RESET(sigemptyset);
    TEST_PASSTHROUGH_RESET(sigfillset);
    TEST_PASSTHROUGH_RESET(sigdelset);
    TEST_PASSTHROUGH_RESET(pthread_sigmask);
    fail_malloc_count = 0;
    track_size = 0;
    tracked_ptr = NULL;
    tracked_freed = 0;
    watch_size = 0;
    watch_count = 0;
    FFF_RESET_HISTORY();
    g_gflag = false;
    g_sig_handled = 0;
    (void)memset(readbuf, 0, sizeof(readbuf));
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
    close_sock(&ssock);
    close_sock(&csock);
}

/**
 * test_set_port_string() 関数テスト
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
 * test_server_sock() 関数テスト
 */
TEST
test_server_sock(void)
{
    /* 準備をして, 関数を実行し, 結果を確認する */
    dbglog("start");

    if (set_port_string(port) < 0)
        TEST_FAIL("set_port_string");
    ssock = server_sock();
    dbglog("server_sock=%d", ssock);

    TEST_ASSERT_NOT_INT(EX_NG, ssock);

    csock = inet_sock_client();
    if (csock < 0)
        TEST_FAIL("inet_sock_client");

    PASS();
}

/**
 * test_server_loop() 関数テスト
 */
TEST
test_server_loop(void)
{
    pid_t cpid = 0; /* 子プロセスID */
    pid_t w = 0;    /* wait戻り値 */
    int status = 0; /* wait引数 */
    int retval = 0; /* 戻り値 */
    int count = 1;  /* ループカウント */

    if (set_port_string(port) < 0)
        TEST_FAIL("set_port_string");
    ssock = server_sock();

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) {
        dbglog("child");

        count = 2;
        g_sig_handled = 1;
        while (count > 0) {
            server_loop(ssock);
            count--;
        }
        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        csock = inet_sock_client();
        if (csock < 0) {
            TEST_FAIL("inet_sock_client");
        }

        /* 送信 */
        retval = send_client(csock, expr, sizeof(expr));
        if (retval < 0) {
            TEST_FAIL("send_client: csock=%d(%d)", csock, errno);
        }

        /* 受信 */
        retval = recv_client(csock, readbuf);
        if (retval < 0) {
            TEST_FAIL("recv_client: csock=%d(%d)", csock, errno);
        }

        TEST_ASSERT_STR((char *)expected, (char *)readbuf);

        w = wait(&status);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
    }
    PASS();
}

/**
 * test_server_proc() 関数テスト
 */
#if 0
TEST
test_server_proc(void)
{
    pid_t cpid = 0;         /* 子プロセスID */
    pid_t w = 0;            /* wait戻り値 */
    int status = 0;         /* wait引数 */
    int retval = 0;         /* 戻り値 */
    thread_data *dt = NULL; /* ソケット情報構造体 */
    void *servret = NULL;   /* テスト関数戻り値 */

    if (set_port_string(port) < 0)
        TEST_FAIL("set_port_string");
    ssock = server_sock();

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) {
        dbglog("child");

        dt = (thread_data *)malloc(sizeof(thread_data));
        if (dt == NULL) {
            outlog("malloc: size=%zu", sizeof(thread_data));
            exit(EXIT_FAILURE);
        }
        (void)memset(dt, 0, sizeof(thread_data));

        dt->len = (socklen_t)sizeof(dt->addr);
        dt->sock = accept(ssock, (struct sockaddr *)&dt->addr, &dt->len);
        if (dt->sock < 0) {
            outlog("accept: ssock=%d", ssock);
            memfree(&dt, NULL);
            exit(EXIT_FAILURE);
        }
        g_sig_handled = 1;

        /* テスト関数実行 */
        servret = server.server_proc(dt);
        if (servret != NULL) {
            outlog("server_proc");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        csock = inet_sock_client();
        if (csock < 0) {
            TEST_FAIL("inet_sock_client");
        }

        /* 送信 */
        retval = send_client(csock, expr, sizeof(expr));
        if (retval < 0) {
            TEST_FAIL("send_client: csock=%d(%d)", csock, errno);
        }

        /* 受信 */
        retval = recv_client(csock, readbuf);
        if (retval < 0) {
            TEST_FAIL("recv_client: csock=%d(%d)", csock, errno);
        }

        TEST_ASSERT_STR((char *)expected, (char *)readbuf);

        w = wait(&status);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        //if (WEXITSTATUS(status))
        //    TEST_FAIL("child failed");
    }
    PASS();
}
#endif

/**
 * 送信
 *
 * @param[in] sockfd ソケット
 * @param[in] sbuf 送信バッファ
 * @param[in] length バイト数
 * @retval EX_NG エラー
 */
static int
send_client(int sockfd, unsigned char *sbuf, size_t length)
{
    struct client_data *cdata = NULL; /* 送信データ構造体 */
    ssize_t slen = 0L;                /* 送信データバイト数 */
    int retval = 0;                   /* 戻り値 */

    dbglog("start");

    /* データ設定 */
    slen = set_client_data(&cdata, sbuf, length);
    if (slen < 0L) {
        TEST_NOTIFY("set_server_data=%zd(%d)", slen, errno);
        return EX_NG;
    }

    /* 送信 */
    retval = send_data(sockfd, cdata, (size_t *)&slen);
    if (retval < 0) {
        TEST_NOTIFY("send_data: slen=%zd(%d)", slen, errno);
        memfree(&cdata, NULL);
        return EX_NG;
    }
    memfree(&cdata, NULL);
    return EX_OK;
}

/**
 * 受信
 *
 * @param[in] sockfd ソケット
 * @param[in] rbuf 受信バッファ
 * @retval EX_NG エラー
 */
static int
recv_client(int sockfd, unsigned char *rbuf)
{
    size_t length = 0u; /* バイト数 */
    struct header hd;   /* ヘッダ構造体 */
    int retval = 0;     /* 戻り値 */

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
 * ソケット作成
 *
 * @return 接続したソケット
 * @retval EX_NG エラー
 */
static int
inet_sock_client(void)
{
    struct sockaddr_in saddr; /* ソケットアドレス情報構造体 */
    int sockfd = 0;           /* ソケット */
    int retval = 0;           /* 戻り値 */

    dbglog("start");

    /* 初期化 */
    (void)memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;

    if (set_hostname(&saddr, hostname) < 0)
        return EX_NG;
    if (set_port(&saddr, port) < 0)
        return EX_NG;

    /* ソケット生成 */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        outlog("sock=%d", sockfd);
        return EX_NG;
    }

    /* コネクト */
    retval = connect(sockfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (retval < 0) {
        outlog("connect=%d, sock=%d", retval, sockfd);
        /* ソケットクローズ */
        close_sock(&sockfd);
        return EX_NG;
    }
    return sockfd;
}

/**
 * シグナル設定
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
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
 * server_sock() 関数テスト (失敗)
 */
TEST
test_server_sock_failure(void)
{
    int sock = -1; /* ソケット */

    /* ポート番号を解決できない (存在しないサービス名. 5文字まで) */
    TEST_ASSERT_INT(EX_OK, set_port_string("nosvc"));
    TEST_ASSERT_INT(EX_NG, server_sock());

    TEST_ASSERT_INT(EX_OK, set_port_string(port));

    /* socket() に失敗 */
    TEST_INJECT(socket, 0, 1, -1, EMFILE);
    TEST_ASSERT_INT(EX_NG, server_sock());
    TEST_ASSERT_INJECTED(socket);

    /* setsockopt() に失敗 */
    TEST_INJECT(setsockopt, 0, 1, -1, EINVAL);
    TEST_ASSERT_INT(EX_NG, server_sock());
    TEST_ASSERT_INJECTED(setsockopt);

    /* listen() に失敗 */
    TEST_INJECT(listen, 0, 1, -1, EOPNOTSUPP);
    TEST_ASSERT_INT(EX_NG, server_sock());
    TEST_ASSERT_INJECTED(listen);

    /* bind() に失敗 (同じポート番号は, 使用中) */
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);
    sock = server_sock();
    TEST_ASSERT_INT(EX_NG, sock);
    PASS();
}

/**
 * server_loop() 関数テスト (失敗)
 */
TEST
test_server_loop_failure(void)
{
    int closed = -1; /* 閉じたファイルディスクリプタ */

    TEST_ASSERT_INT(EX_OK, set_port_string(port));

    /* ノンブロッキングに設定できない (閉じたファイルディスクリプタ) */
    closed = dup(STDOUT_FILENO);
    if (closed < 0) {
        TEST_FAIL("dup(%d)", errno);
    }
    (void)close(closed);
    server_loop(closed);
    TEST_ASSERT_INT(0, pselect_fake.call_count);

    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);

    /* pselect() が割り込まれた */
    TEST_INJECT(pselect, 0, 1, -1, EINTR);
    server_loop(ssock);
    TEST_ASSERT_INJECTED(pselect);

    /* pselect() に失敗 */
    TEST_INJECT(pselect, 0, 1, -1, EBADF);
    server_loop(ssock);
    TEST_ASSERT_INJECTED(pselect);

    /* accept() に失敗 (接続待ちがないのに, 受付可能と見なす) */
    g_sig_handled = 1; /* 1 回で, ループを終了する */
    TEST_INJECT(pselect, 0, 1, 1, 0);
    server_loop(ssock);
    TEST_ASSERT_INT(1, accept_fake.call_count);

    /* pthread_create() に失敗 */
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    TEST_INJECT(pthread_create, 0, 1, EAGAIN, 0);
    server_loop(ssock);
    TEST_ASSERT_INJECTED(pthread_create);
    close_sock(&csock);

    /* pthread_detach() に失敗 */
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    TEST_INJECT(pthread_detach, 0, 1, ESRCH, 0);
    server_loop(ssock);
    TEST_ASSERT_INJECTED(pthread_detach);
    close_sock(&csock);
    (void)usleep(THREAD_WAIT); /* スレッドが終了するのを待つ */
    PASS();
}

/**
 * server_proc() 関数テスト (失敗と, デバッグ出力)
 * server_proc() は, スレッドで実行されるので, 接続の受付後に, 終了を待つ.
 */
TEST
test_server_proc_failure(void)
{
    struct header hd;             /* ヘッダ */
    unsigned char rbuf[BUF_SIZE]; /* 受信バッファ */

    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);
    g_sig_handled = 1; /* server_loop() は, 1 回で終了する */

    /* ヘッダを受信できない (何も送らずに閉じる) */
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    server_loop(ssock);
    close_sock(&csock);
    (void)usleep(THREAD_WAIT);

    /* データを受信できない (ヘッダだけ送って閉じる) */
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    (void)memset(&hd, 0, sizeof(hd));
    hd.length = htonl(4u);
    TEST_ASSERT_INT(sizeof(hd), writen(csock, &hd, sizeof(hd)));
    server_loop(ssock);
    close_sock(&csock);
    (void)usleep(THREAD_WAIT);

    /* データ長が 0 */
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    hd.length = htonl(0u);
    TEST_ASSERT_INT(sizeof(hd), writen(csock, &hd, sizeof(hd)));
    server_loop(ssock);
    close_sock(&csock);
    (void)usleep(THREAD_WAIT);

    /* 正常なリクエストで, デバッグ出力 (-g) */
    g_gflag = true;
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    TEST_ASSERT_INT(EX_OK, send_client(csock, expr, sizeof(expr)));
    server_loop(ssock);
    (void)memset(rbuf, 0, sizeof(rbuf));
    TEST_ASSERT_INT(EX_OK, recv_client(csock, rbuf));
    TEST_ASSERT_STR((char *)expected, (char *)rbuf);
    close_sock(&csock);
    (void)usleep(THREAD_WAIT);
    PASS();
}

/**
 * server_loop() 関数テスト (メモリを確保できない)
 */
TEST
test_server_loop_alloc_failure(void)
{
    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);

    /* 接続待ちがあるので, 受付可能になるが, スレッドデータを作れない */
    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    g_sig_handled = 1; /* 1 回で, ループを終了する */
    fail_malloc_size = sizeof(thread_data);
    fail_malloc_count = 1;
    server_loop(ssock);
    TEST_ASSERT_INT(0, fail_malloc_count);
    PASS();
}

/**
 * server_loop() 関数テスト (シグナルマスクの取得に失敗)
 */
TEST
test_server_loop_signal_mask_failure(void)
{
    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);

    /* 失敗しても, 続行する (sigdelset() は, 2 回呼ばれる) */
    TEST_INJECT(sigemptyset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigfillset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigdelset, 0, 2, -1, EINVAL);
    TEST_INJECT(pselect, 0, 1, -1, EINTR);
    server_loop(ssock);
    TEST_ASSERT_INJECTED(sigemptyset);
    TEST_ASSERT_INJECTED(sigfillset);
    TEST_ASSERT_INJECTED(sigdelset);
    PASS();
}

/**
 * server_proc() 関数テスト (内部の関数の失敗)
 * 有効なリクエストを送って, スレッドの中の失敗を注入する.
 */
TEST
test_server_proc_internal_failure(void)
{
    unsigned int i;

    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);
    g_sig_handled = 1; /* server_loop() は, 1 回で終了する */

    for (i = 0u; i < 4u; i++) {
        csock = inet_sock_client();
        TEST_ASSERT_NOT_INT(EX_NG, csock);
        TEST_ASSERT_INT(EX_OK, send_client(csock, expr, sizeof(expr)));
        switch (i) {
        case 0: /* シグナルマスクの設定に失敗しても, 続行する */
            TEST_INJECT(pthread_sigmask, 0, 1, EINVAL, 0);
            break;
        case 1: /* 計算に失敗 */
            TEST_INJECT(create_answer, 0, 1, NULL, ENOMEM);
            break;
        case 2: /* 送信データを作れない */
            TEST_INJECT(set_server_data, 0, 1, EX_NG, ENOMEM);
            break;
        default: /* 送信に失敗 */
            TEST_INJECT(send_data, 0, 1, EX_NG, EPIPE);
            break;
        }
        server_loop(ssock);
        (void)usleep(THREAD_WAIT);
        close_sock(&csock);
    }
    TEST_ASSERT_INJECTED(send_data);
    PASS();
}

/**
 * server_proc() 関数テスト (引数として渡されたスレッドデータを解放する)
 */
TEST
test_server_proc_free_arg(void)
{
    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);
    g_sig_handled = 1; /* server_loop() は, 1 回で終了する */

    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    track_size = sizeof(thread_data);
    server_loop(ssock);
    close_sock(&csock); /* スレッドは, ヘッダを受信できずに終了する */
    (void)usleep(THREAD_WAIT);

    TEST_ASSERT_MSG(tracked_ptr != NULL, "thread_data was not allocated");
    TEST_ASSERT_MSG(tracked_freed, "thread_data was not freed");
    PASS();
}

/**
 * server_proc() 関数テスト (データ長の上限)
 * 上限を超えるデータ長のヘッダを受け取っても, そのサイズのメモリを確保しない.
 */
TEST
test_server_proc_length_limit(void)
{
    struct header hd; /* ヘッダ */

    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);
    g_sig_handled = 1; /* server_loop() は, 1 回で終了する */

    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    (void)memset(&hd, 0, sizeof(hd));
    hd.length = htonl(MAX_DATA_LENGTH + 1u);
    TEST_ASSERT_INT(sizeof(hd), writen(csock, &hd, sizeof(hd)));
    watch_size = MAX_DATA_LENGTH + 1u;
    server_loop(ssock);
    (void)usleep(THREAD_WAIT);
    close_sock(&csock);

    TEST_ASSERT_INT(0, watch_count);
    PASS();
}

/**
 * server_proc() 関数テスト (終端の NUL がない式)
 * 終端のない式を受け取っても, 確保した領域の外を読まずに, 計算できる.
 */
TEST
test_server_proc_no_nul(void)
{
    struct header hd;                                                  /* ヘッダ */
    unsigned char noterm[] = {'1', '+', '1', '+', '1', '+', '1', '+'}; /* 終端の NUL がない式 */
    unsigned char rbuf[BUF_SIZE];                                      /* 受信バッファ */

    TEST_ASSERT_INT(EX_OK, set_port_string(port));
    ssock = server_sock();
    TEST_ASSERT_NOT_INT(EX_NG, ssock);
    g_sig_handled = 1; /* server_loop() は, 1 回で終了する */

    csock = inet_sock_client();
    TEST_ASSERT_NOT_INT(EX_NG, csock);
    (void)memset(&hd, 0, sizeof(hd));
    hd.length = htonl(sizeof(noterm));
    TEST_ASSERT_INT(sizeof(hd), writen(csock, &hd, sizeof(hd)));
    TEST_ASSERT_INT(sizeof(noterm), writen(csock, noterm, sizeof(noterm)));
    server_loop(ssock);

    /* 最後の 1 バイトが NUL に置き換えられて, "1+1+1+1" として計算される */
    (void)memset(rbuf, 0, sizeof(rbuf));
    TEST_ASSERT_INT(EX_OK, recv_client(csock, rbuf));
    TEST_ASSERT_STR("4", (char *)rbuf);
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
    RUN_TEST(test_set_port_string);
    RUN_TEST(test_server_sock);
    RUN_TEST(test_server_loop);
    RUN_TEST(test_server_sock_failure);
    RUN_TEST(test_server_loop_failure);
    RUN_TEST(test_server_proc_failure);
    RUN_TEST(test_server_loop_alloc_failure);
    RUN_TEST(test_server_loop_signal_mask_failure);
    RUN_TEST(test_server_proc_internal_failure);
    RUN_TEST(test_server_proc_free_arg);
    RUN_TEST(test_server_proc_length_limit);
    RUN_TEST(test_server_proc_no_nul);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}

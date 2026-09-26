/**
 * @file tests/lib/test_net.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-15 higashi 新規作成
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

#include <stdio.h>     /* snprintf */
#include <stdlib.h>    /* exit */
#include <unistd.h>    /* access fork */
#include <fcntl.h>     /* open fcntl */
#include <arpa/inet.h> /* inet_ntoa */
#include <sys/stat.h>  /* chmod */
#include <stdarg.h>    /* va_list */
#include <sys/socket.h> /* socketpair send recv */
#include <sys/un.h>    /* sockaddr_un */
#include <sys/wait.h>  /* wait waitpid */
#include <errno.h>     /* errno */
#include <signal.h>    /* signal */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "fileio.h"
#include "net.h"

#define BUF_SIZE 2048

DEFINE_FFF_GLOBALS;

/* send() と recv() は, モックにして, 通常は本物を呼ぶ (EINTR などを注入する) */
FAKE_VALUE_FUNC(ssize_t, send, int, const void *, size_t, int);
TEST_PASSTHROUGH(ssize_t, send, (int fd, const void *buf, size_t n, int flags),
                 (fd, buf, n, flags))
FAKE_VALUE_FUNC(ssize_t, recv, int, void *, size_t, int);
TEST_PASSTHROUGH(ssize_t, recv, (int fd, void *buf, size_t n, int flags),
                 (fd, buf, n, flags))

/*
 * fcntl() は, 可変引数なので, FFF のモックにはせず, F_SETFL のときだけ失敗させる.
 * 本物は dlsym(RTLD_NEXT) で探す.
 */
static struct test_inject inject_fcntl; /**< fcntl() (F_SETFL) に注入する失敗 */
static int (*real_fcntl)(int, int, ...) = NULL; /**< 本物の fcntl() */
/**
 * fcntl() の置き換え
 *
 * @param[in] fd ファイルディスクリプタ
 * @param[in] cmd コマンド
 * @return fcntl() の戻り値. F_SETFL は, 注入した失敗のときは, その値
 */
int
fcntl(int fd, int cmd, ...)
{
    va_list ap; /* 可変引数 */
    long arg = 0; /* fcntl の引数 */

    /* 本物の fcntl() を探す */
    if (!real_fcntl)
        *(void **)(&real_fcntl) = dlsym(RTLD_NEXT, "fcntl");
    va_start(ap, cmd);
    arg = va_arg(ap, long);
    va_end(ap);
    /* F_SETFL のときだけ, 注入した失敗を返す */
    if (cmd == F_SETFL && inject_fcntl.count > 0) {
        inject_fcntl.count--;
        errno = inject_fcntl.err;
        return (int)inject_fcntl.value;
    }
    return real_fcntl(fd, cmd, arg);
}

/* プロトタイプ */
/** set_hostname() 関数テスト */
TEST test_set_hostname(void);
/** set_port() 関数テスト */
TEST test_set_port(void);
/** set_block() 関数テスト */
TEST test_set_block(void);
/** send_data() 関数テスト */
TEST test_send_data(void);
/** recv_data() 関数テスト */
TEST test_recv_data(void);
/** recv_data_new() 関数テスト */
TEST test_recv_data_new(void);
/** close_sock() 関数テスト */
TEST test_close_sock(void);
/** set_hostname() 関数テスト (失敗) */
TEST test_set_hostname_failure(void);
/** set_port() 関数テスト (失敗) */
TEST test_set_port_failure(void);
/** set_block() 関数テスト (失敗) */
TEST test_set_block_failure(void);
/** send_data() 関数テスト (失敗) */
TEST test_send_data_failure(void);
/** send_data() 関数テスト (EINTR, EAGAIN) */
TEST test_send_data_interrupted(void);
/** recv_data() 関数テスト (失敗) */
TEST test_recv_data_failure(void);
/** recv_data() 関数テスト (EINTR, EAGAIN) */
TEST test_recv_data_interrupted(void);
/** recv_data_new() 関数テスト (失敗) */
TEST test_recv_data_new_failure(void);
/** close_sock() 関数テスト (失敗) */
TEST test_close_sock_failure(void);

/* 内部変数 */
static char sockfile[TEST_TMPNAME_SIZE] = {0}; /**< ソケットファイル */
static struct sockaddr_un addr;       /**< sockaddr_un構造体 */
static socklen_t addrlen = 0;         /**< addr構造体の長さ */
static char command[] = "do send";    /**< コマンド */
static int ssock = -1;                /**< サーバソケット */
static int csock = -1;                /**< クライアントソケット */
static int acc = -1;                  /**< アクセプト */
static int fd = -1;                   /**< ファイルディスクリプタ */
static char *readnew = NULL;          /**< クライアント受信用ポインタ */
static char sendbuf[BUF_SIZE];        /**< 送信バッファ */

/* 内部関数 */
/** サーバプロセス */
static int server_proc(int sockfd, char *readbuf, size_t length);
/** サーバソケット生成 */
static int unix_sock_server(void);
/** クライアントソケット生成 */
static int unix_sock_client(void);
/** シグナル設定 */
static void set_sig_handler(void);

/**
 * 初期化処理
 */
static void
startup(void)
{
    set_sig_handler();

    /* ソケットファイル文字列設定 */
    if (test_tmpname(sockfile) < 0) {
        TEST_ERROR("test_tmpname(%d)", errno);
        exit(EXIT_FAILURE);
    }

    /* sockaddr_un構造体の設定 */
    (void)memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    (void)strncpy(addr.sun_path, sockfile, sizeof(addr.sun_path) - 1);
    addrlen = sizeof(addr.sun_family) + strlen(addr.sun_path);
}

/**
 * 初期化処理
 *
 * @param[in] data 使用しない
 */
static void
setup(void *data)
{
    TEST_PASSTHROUGH_RESET(send);
    (void)memset(&inject_fcntl, 0, sizeof(inject_fcntl));
    TEST_PASSTHROUGH_RESET(recv);
    (void)memset(sendbuf, 'a', sizeof(sendbuf));
    sendbuf[sizeof(sendbuf) - 1] = '\0';
    sendbuf[sizeof(sendbuf) - 2] = '\n';
}

/**
 * 終了処理
 *
 * @param[in] data 使用しない
 */
static void
teardown(void *data)
{
    if (acc != -1) {
        if (close(acc) < 0)
            TEST_NOTIFY("close: acc=%d(%d)", acc, errno);
        acc = -1;
    }
    if (ssock != -1) {
        if (close(ssock) < 0)
            TEST_NOTIFY("close: ssock=%d(%d)", ssock, errno);
        ssock = -1;
    }
    if (csock != -1) {
        if (close(csock) < 0)
            TEST_NOTIFY("close: csock=%d(%d)", csock, errno);
        csock = -1;
    }
    if (fd != -1) {
        if (close(fd) < 0)
            TEST_NOTIFY("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }

    if (sockfile[0] != '\0') {
        if (!access(sockfile, W_OK)) { /* ファイルが存在する */
            if (unlink(sockfile) < 0)
                TEST_NOTIFY("unlink: %s(%d)", sockfile, errno);
        }
    }

    if (readnew)
        free(readnew);
    readnew = NULL;
}

/**
 * set_hostname() 関数テスト
 */
TEST
test_set_hostname(void)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    int retval = 0;            /* 戻り値 */

    /* テストデータ */
    const char *host[] = { "localhost", "127.0.0.1" }; /* ホスト文字列 */
    const char *ipaddr = "127.0.0.1";                  /* アドレス */
    const char nohost[] = "nohostxhlkjiherlgfsd";      /* エラー用データ */

    /* 正常系 */
    unsigned int i;
    for (i = 0; i < NELEMS(host); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));

        retval = set_hostname(&server, host[i]);

        TEST_ASSERT_STR_MSG(ipaddr, inet_ntoa(server.sin_addr), "expected=%s, actual=%s",
                                            ipaddr,
                                            inet_ntoa(server.sin_addr));
        TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");
    }

    /* 異常系 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));

    retval = set_hostname(&server, nohost);

    TEST_ASSERT_INT_MSG(EX_NG, retval, "return value");
    PASS();
}

/**
 * set_port() 関数テスト
 */
TEST
test_set_port(void)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    int retval = 0;            /* 戻り値 */

    /* テストデータ */
    const char *port[] = { "1", "65534", "65535", "ftp" }; /* ポート文字列 */
    const uint32_t portno[] = { 1, 65534, 65535, 21 };     /* ポート番号 */
    const char *err_port[] = { "0", "65536", "noservice" }; /* エラー */

    /* 正常系 */
    unsigned int i;
    for (i = 0; i < NELEMS(port); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));

        retval = set_port(&server, port[i]);
        TEST_ASSERT_INT_MSG((unsigned int)portno[i], (unsigned int)ntohs((uint16_t)server.sin_port), "expected=%u, actual=%u",
                                          portno[i], ntohs(server.sin_port));
        TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");
    }

    /* 異常系 */
    for (i = 0; i < NELEMS(err_port); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));

        retval = set_port(&server, err_port[i]);
        TEST_ASSERT_INT_MSG(EX_NG, retval, "return value");
    }
    PASS();
}

/**
 * set_block() 関数テスト
 */
TEST
test_set_block(void)
{
    int retval = 0; /* テスト関数戻り値 */
    int flags = 0;  /* fcntl戻り値 */

    fd = open("/dev/null", O_RDWR, 0);
    if (fd < 0) {
        TEST_FAIL("open=%d", fd);
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        TEST_FAIL("fcntl=%d", flags);
    }
    dbglog("flags=%d", flags);

    /* 正常系 */
    /* ノンブロッキング */
    retval = set_block(fd, NONBLOCK);

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        TEST_FAIL("fcntl=%d", flags);
    }

    TEST_ASSERT_INT(O_NONBLOCK, flags & O_NONBLOCK);
    TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");

    /* ブロッキング */
    retval = set_block(fd, BLOCKING);

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        TEST_FAIL("fcntl=%d", flags);
    }

    TEST_ASSERT_INT(0, flags & O_NONBLOCK);
    TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");

    /* 異常系 */
    retval = set_block(fd, (blockmode)2);
    TEST_ASSERT_INT_MSG(EX_NG, retval, "return value");
    PASS();
}

/**
 * send_data() 関数テスト
 */
TEST
test_send_data(void)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* バイト数 */
    pid_t cpid = 0;    /* 子プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char servbuf[sizeof(sendbuf)]; /* サーバ受信用バッファ */

    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        TEST_FAIL("unix_sock_server");
    }

    cpid = fork();
    if (cpid < 0) { /* エラー */
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        retval = server_proc(ssock, servbuf, sizeof(servbuf));
        if (retval < 0) {
            outlog("server_proc: ssock", ssock);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        csock = unix_sock_client();
        if (csock < 0) {
            TEST_FAIL("unix_sock_client");
        }

        /* テスト関数の実行 */
        length = sizeof(sendbuf);
        retval = send_data(csock, sendbuf, &length);
        dbglog("send_data=%d, %s", retval, sendbuf);
        TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            TEST_FAIL("status=%d", WEXITSTATUS(status));
    }

    PASS();
}

/**
 * recv_data() 関数テスト
 */
TEST
test_recv_data(void)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* バイト数 */
    ssize_t len = 0;   /* 送信されたバイト数 */
    pid_t cpid = 0;    /* プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char readbuf[sizeof(sendbuf)]; /* クライアント受信用バッファ */
    char servbuf[sizeof(command)]; /* サーバ受信用バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));
    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        TEST_FAIL("unix_sock_server");
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        retval = server_proc(ssock, servbuf, sizeof(servbuf));
        if (retval < 0) {
            outlog("server_proc: ssock", ssock);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        csock = unix_sock_client();
        if (csock < 0) {
            TEST_FAIL("unix_sock_client");
        }

        len = writen(csock, command, sizeof(command));
        if (len < 0) {
            TEST_FAIL("writen=%zd(%d)", len, errno);
        }

        /* テスト関数の実行 */
        length = sizeof(readbuf);
        retval = recv_data(csock, readbuf, &length);

        TEST_ASSERT_MEM_MSG(sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf), "%s==%s", sendbuf, readbuf);
        TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            TEST_FAIL("status=%d", WEXITSTATUS(status));
    }
    PASS();
}

/**
 * recv_data_new() 関数テスト
 */
TEST
test_recv_data_new(void)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* バイト数 */
    ssize_t len = 0;   /* 送信されたバイト数 */
    pid_t cpid = 0;    /* プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char servbuf[sizeof(command)]; /* サーバ受信用バッファ */

    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        TEST_FAIL("unix_sock_server");
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        retval = server_proc(ssock, servbuf, sizeof(servbuf));
        if (retval < 0) {
            outlog("server_proc: ssock", ssock);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        csock = unix_sock_client();
        if (csock < 0) {
            TEST_FAIL("unix_sock_client");
        }

        len = writen(csock, command, sizeof(command));
        if (len < 0) {
            TEST_FAIL("writen=%zd(%d)", len, errno);
        }

        /* テスト関数の実行 */
        length = sizeof(sendbuf);
        readnew = (char *)recv_data_new(csock, &length);

        TEST_ASSERT_MEM_MSG(sendbuf, sizeof(sendbuf), readnew, length, "%s==%s", sendbuf, readnew);
        TEST_ASSERT_INT_MSG(EX_OK, retval, "return value");
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            TEST_FAIL("status=%d", WEXITSTATUS(status));
    }
    PASS();
}

/**
 * close_sock() 関数テスト
 */
TEST
test_close_sock(void)
{

    int retval = 0;    /* 戻り値 */
    ssize_t len = 0;   /* 送信されたバイト数 */
    pid_t cpid = 0;    /* プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char servbuf[sizeof(sendbuf)]; /* サーバ受信用バッファ */

    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        TEST_FAIL("unix_sock_server");
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_FAIL("fork(%d)", errno);
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        retval = server_proc(ssock, servbuf, sizeof(servbuf));
        if (retval < 0) {
            outlog("server_proc: ssock", ssock);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        csock = unix_sock_client();
        if (csock < 0) {
            TEST_FAIL("unix_sock_client");
        }

        len = writen(csock, sendbuf, sizeof(sendbuf));
        if (len < 0) {
            TEST_FAIL("writen=%zd(%d)", len, errno);
        }

        /* テスト関数の実行 */
        /* 正常系 */
        retval = close_sock(&csock);
        TEST_ASSERT_INT(-1, csock);
        TEST_ASSERT_INT(EX_OK, retval);
        /* -1のときはなにもしない */
        retval = close_sock(&csock);

        TEST_ASSERT_INT(EX_OK, retval);

        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            TEST_FAIL("status=%d", WEXITSTATUS(status));
    }
    PASS();
}

/**
 * サーバプロセス
 *
 * @param[in] sockfd ソケット
 * @param[in] readbuf 受信バッファ
 * @param[in] length バッファ長さ
 * @retval EX_NG エラー
 */
static int
server_proc(int sockfd, char *readbuf, size_t length)
{
    socklen_t len = 0; /* sockaddr構造体長さ */
    ssize_t rlen = 0;  /* 受信された長さ */
    ssize_t wlen = 0;  /* 送信された長さ */
    int retval = 0;    /* 戻り値 */

    dbglog("start: sockfd=%d", sockfd);

    len = addrlen;
    acc = accept(sockfd, (struct sockaddr *)&addr, &len);
    if (acc < 0) {
        outlog("accept: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }
    dbglog("accept=%d, sockfd=%d(%d)", sockfd, errno);

    rlen = readn(acc, readbuf, length);
    if (rlen < 0) {
        outlog("readn: acc=%d(%d)", acc, errno);
        return EX_NG;
    }

    retval = strncmp(readbuf, command, strlen(command));
    dbglog("strncmp=%d, %s==%s", retval, readbuf, command);
    if (retval == 0) { /* 送信 */
        wlen = writen(acc, sendbuf, sizeof(sendbuf));
        if (wlen < 0) {
            outlog("writen: acc=%d(%d)", acc, errno);
            return EX_NG;
        }
    } else {
        retval = memcmp(sendbuf, readbuf, sizeof(sendbuf));
        if (retval) { /* 非0 */
            /* 受信されたデータが不一致な場合, エラー */
            outlog("memcmp: sendbuf=%p, readbuf=%p, len=%zu(%d)",
                   sendbuf, readbuf, sizeof(sendbuf), errno);
            return EX_NG;
        }
    }
    return EX_OK;
}

/**
 * サーバソケット生成
 *
 * @return ソケット
 * @retval EX_NG エラー
 */
static int
unix_sock_server(void)
{
    int retval = 0;     /* 戻り値 */
    int sockfd = 0;     /* ソケット */
    const mode_t mode = /* ファイルの許可 */
        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

    dbglog("start");

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        TEST_NOTIFY("socket(%d)", errno);
        return EX_NG;
    }

    retval = bind(sockfd, (struct sockaddr *)&addr, addrlen);
    if (retval < 0) {
        if (errno == EADDRINUSE)
            TEST_NOTIFY("Address already in use");
        TEST_NOTIFY("bind: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    retval = listen(sockfd, SOMAXCONN);
    if (retval < 0) {
        TEST_NOTIFY("listen sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    retval = chmod(sockfile, mode);
    if (retval < 0) {
        TEST_NOTIFY("chmod: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    return sockfd;
}

/**
 * クライアントソケット生成
 *
 * @return ソケット
 * @retval EX_NG エラー
 */
static int
unix_sock_client(void)
{
    int retval = 0; /* 戻り値 */
    int sockfd = 0; /* ソケット */

    dbglog("start");

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        TEST_NOTIFY("socket(%d)", errno);
        return EX_NG;
    }

    retval = connect(sockfd, (struct sockaddr *)&addr, addrlen);
    if (retval < 0) {
        TEST_NOTIFY("connect=%d(%d)", sockfd, errno);
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


/**
 * set_hostname() 関数テスト (失敗)
 */
TEST
test_set_hostname_failure(void)
{
    struct sockaddr_in addr; /* sockaddr_in構造体 */

    (void)memset(&addr, 0, sizeof(addr));

    TEST_ASSERT_INT(EX_NG, set_hostname(NULL, "127.0.0.1"));
    TEST_ASSERT_INT(EX_NG, set_hostname(&addr, NULL));
    /* IPアドレスでも, ホスト名でもない */
    TEST_ASSERT_INT(EX_NG, set_hostname(&addr, ""));
    PASS();
}

/**
 * set_port() 関数テスト (失敗)
 */
TEST
test_set_port_failure(void)
{
    struct sockaddr_in addr; /* sockaddr_in構造体 */

    (void)memset(&addr, 0, sizeof(addr));

    TEST_ASSERT_INT(EX_NG, set_port(NULL, "12345"));
    TEST_ASSERT_INT(EX_NG, set_port(&addr, NULL));
    /* 存在しないサービス名 */
    TEST_ASSERT_INT(EX_NG, set_port(&addr, "no-such-service"));

    /* 範囲外 (65536 以上は, uint16_t に切り捨てられて, 別のポート番号にならない) */
    TEST_ASSERT_INT(EX_NG, set_port(&addr, "0"));
    TEST_ASSERT_INT(EX_NG, set_port(&addr, "65536"));
    TEST_ASSERT_INT(EX_NG, set_port(&addr, "65616"));
    TEST_ASSERT_INT(EX_NG, set_port(&addr, "70000"));
    TEST_ASSERT_INT(EX_NG, set_port(&addr, "4294967376"));

    /* 範囲内の最小と最大 */
    TEST_ASSERT_INT(EX_OK, set_port(&addr, "1"));
    TEST_ASSERT_INT(1, ntohs(addr.sin_port));
    TEST_ASSERT_INT(EX_OK, set_port(&addr, "65535"));
    TEST_ASSERT_INT(65535, ntohs(addr.sin_port));
    PASS();
}

/**
 * set_block() 関数テスト (失敗)
 */
TEST
test_set_block_failure(void)
{
    int devnull = -1; /* ファイルディスクリプタ */

    /* 不正なファイルディスクリプタ */
    TEST_ASSERT_INT(EX_NG, set_block(-1, NONBLOCK));

    /* 不正なモード */
    devnull = open("/dev/null", O_RDWR, 0);
    if (devnull < 0) {
        TEST_FAIL("open(%d)", errno);
    }
    TEST_ASSERT_INT(EX_NG, set_block(devnull, (blockmode)99));

    /* F_SETFL に失敗すると, エラーを返す */
    inject_fcntl.count = 1;
    inject_fcntl.value = -1;
    inject_fcntl.err = EBADF;
    TEST_ASSERT_INT(EX_NG, set_block(devnull, NONBLOCK));
    TEST_ASSERT_INT(0, inject_fcntl.count);
    inject_fcntl.count = 1;
    TEST_ASSERT_INT(EX_NG, set_block(devnull, BLOCKING));
    TEST_ASSERT_INT(0, inject_fcntl.count);
    (void)close(devnull);
    PASS();
}

/**
 * send_data() 関数テスト (失敗)
 */
TEST
test_send_data_failure(void)
{
    int sv[2] = { -1, -1 }; /* ソケットペア */
    size_t length = 4;      /* 送信バイト数 */
    void (*oldsig)(int);    /* 元のシグナルハンドラ */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }

    /* 接続先が閉じている (EPIPE) */
    oldsig = signal(SIGPIPE, SIG_IGN);
    (void)close(sv[1]);
    TEST_ASSERT_INT(EX_NG, send_data(sv[0], "abcd", &length));
    TEST_ASSERT_INT(0, length);
    (void)signal(SIGPIPE, oldsig);
    (void)close(sv[0]);
    PASS();
}

/**
 * send_data() 関数テスト (EINTR, EAGAIN)
 */
TEST
test_send_data_interrupted(void)
{
    int sv[2] = { -1, -1 }; /* ソケットペア */
    size_t length = 0;      /* 送信バイト数 */
    char readbuf[8] = {0};  /* 受信バッファ */
    const int errnos[] = { EINTR, EAGAIN }; /* 注入する errno */

#ifdef _DEBUG
    /* デバッグビルドの dbglog() (system_dbg_log) は, errno を 0 にするので,
     * send() の直後の errno の判定 (EINTR, EAGAIN) が, 働かない */
    SKIPm("dbglog() clears errno in debug builds");
#endif

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }

    /* 1 回目は失敗するが, やり直して, 送信できる */
    unsigned int i;
    for (i = 0; i < NELEMS(errnos); i++) {
        length = 3;
        (void)memset(readbuf, 0, sizeof(readbuf));
        RESET_FAKE(send);
        send_fake.custom_fake = pass_send;
        TEST_INJECT(send, 0, 1, -1, errnos[i]);
        TEST_ASSERT_INT(EX_OK, send_data(sv[0], "abc", &length));
        TEST_ASSERT_INT(3, length);
        TEST_ASSERT_INT(2, send_fake.call_count);
        TEST_ASSERT_INT(3, read(sv[1], readbuf, sizeof(readbuf)));
        TEST_ASSERT_STR("abc", readbuf);
    }
    (void)close(sv[0]);
    (void)close(sv[1]);
    PASS();
}

/**
 * recv_data() 関数テスト (失敗)
 */
TEST
test_recv_data_failure(void)
{
    int sv[2] = { -1, -1 }; /* ソケットペア */
    size_t length = 4;      /* 受信バイト数 */
    char readbuf[8] = {0};  /* 受信バッファ */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }

    /* 接続先がシャットダウンした */
    (void)close(sv[1]);
    TEST_ASSERT_INT(EX_NG, recv_data(sv[0], readbuf, &length));
    TEST_ASSERT_INT(0, length);
    (void)close(sv[0]);

    /* 不正なソケット */
    length = 4;
    TEST_ASSERT_INT(EX_NG, recv_data(-1, readbuf, &length));
    PASS();
}

/**
 * recv_data() 関数テスト (EINTR, EAGAIN)
 */
TEST
test_recv_data_interrupted(void)
{
    int sv[2] = { -1, -1 }; /* ソケットペア */
    size_t length = 0;      /* 受信バイト数 */
    char readbuf[8] = {0};  /* 受信バッファ */
    const int errnos[] = { EINTR, EAGAIN }; /* 注入する errno */

#ifdef _DEBUG
    /* デバッグビルドの dbglog() (system_dbg_log) は, errno を 0 にするので,
     * send() の直後の errno の判定 (EINTR, EAGAIN) が, 働かない */
    SKIPm("dbglog() clears errno in debug builds");
#endif

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }

    /* 1 回目は失敗するが, やり直して, 受信できる */
    unsigned int i;
    for (i = 0; i < NELEMS(errnos); i++) {
        length = 3;
        (void)memset(readbuf, 0, sizeof(readbuf));
        if (write(sv[1], "abc", 3) != 3) {
            TEST_FAIL("write(%d)", errno);
        }
        RESET_FAKE(recv);
        recv_fake.custom_fake = pass_recv;
        TEST_INJECT(recv, 0, 1, -1, errnos[i]);
        TEST_ASSERT_INT(EX_OK, recv_data(sv[0], readbuf, &length));
        TEST_ASSERT_INT(3, length);
        TEST_ASSERT_INT(2, recv_fake.call_count);
        TEST_ASSERT_STR("abc", readbuf);
    }
    (void)close(sv[0]);
    (void)close(sv[1]);
    PASS();
}

/**
 * recv_data_new() 関数テスト (失敗)
 */
TEST
test_recv_data_new_failure(void)
{
    int sv[2] = { -1, -1 }; /* ソケットペア */
    size_t length = 0;      /* 受信バイト数 */
    void *data = NULL;      /* 受信データ */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        TEST_FAIL("socketpair(%d)", errno);
    }

    /* メモリを確保できない */
    length = (size_t)-1 / 2;
    data = recv_data_new(sv[0], &length);
    TEST_ASSERT_NULL(data);

    /* 受信に失敗 (接続先がシャットダウンした) */
    (void)close(sv[1]);
    length = 4;
    data = recv_data_new(sv[0], &length);
    TEST_ASSERT_NULL(data);
    TEST_ASSERT_INT(0, length);
    (void)close(sv[0]);
    PASS();
}

/**
 * close_sock() 関数テスト (失敗)
 */
TEST
test_close_sock_failure(void)
{
    int sock = 9999; /* オープンしていない */

    TEST_ASSERT_INT(EX_NG, close_sock(&sock));
    TEST_ASSERT_INT(9999, sock);
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
    RUN_TEST(test_set_hostname);
    RUN_TEST(test_set_port);
    RUN_TEST(test_set_block);
    RUN_TEST(test_send_data);
    RUN_TEST(test_recv_data);
    RUN_TEST(test_recv_data_new);
    RUN_TEST(test_close_sock);
    RUN_TEST(test_set_hostname_failure);
    RUN_TEST(test_set_port_failure);
    RUN_TEST(test_set_block_failure);
    RUN_TEST(test_send_data_failure);
    RUN_TEST(test_send_data_interrupted);
    RUN_TEST(test_recv_data_failure);
    RUN_TEST(test_recv_data_interrupted);
    RUN_TEST(test_recv_data_new_failure);
    RUN_TEST(test_close_sock_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}

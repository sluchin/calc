/**
 * @file lib/tests/test_net.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-15 higashi 新規作成
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

#include <stdio.h>     /* tmpnam */
#include <unistd.h>    /* dup2 fork STDERR_FILENO */
#include <sys/wait.h>  /* waitpid */
#include <fcntl.h>     /* open fcntl */
#include <arpa/inet.h> /* inet_ntoa */
#include <sys/stat.h>  /* chmod */
#include <errno.h>     /* errno */
#include <pthread.h>   /* pthread */
#include <cutter.h>    /* cutter library */

#include "def.h"
#include "log.h"
#include "net.h"
#include "test_common.h"

#define MAX_BUF_SIZE 2048

/* プロトタイプ */
/** set_hostname() 関数テスト */
void test_set_hostname(void);
/** set_port() 関数テスト */
void test_set_port(void);
/** set_block() 関数テスト */
void test_set_block(void);
/** send_data() 関数テスト */
void test_send_data(void);
/** recv_data() 関数テスト */
void test_recv_data(void);
/** recv_data_new() 関数テスト */
void test_recv_data_new(void);
/** close_sock() 関数テスト */
void test_close_sock(void);

/* 内部変数 */
static const int EX_ERROR = -1;       /**< エラー戻り値 */
static char sockfile[L_tmpnam] = {0}; /**< ソケットファイル */
static struct sockaddr_un addr;       /**< sockaddr_un構造体 */
static socklen_t addrlen = 0;         /**< addr構造体の長さ */
static char command[] = "do send";    /**< コマンド */
/** 送信バッファ */
const char sendbuf[] = "testsdfgdkslkjwelkjlkjkdjkjglkj";

/* 内部関数 */
/** サーバプロセス */
static void server_proc(int sockfd, char *readbuf, size_t length);
/** サーバソケット生成 */
static int unix_sock_server(void);
/** クライアントソケット生成 */
static int unix_sock_client(void);

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_startup(void)
{
    int fd = 0;          /* ディスクリプタ */
    int retval = 0;      /* 戻り値 */
    char *tmpret = NULL; /* tmpnam戻り値 */
    struct sigaction sa; /* シグナル */

    /* ゾンビプロセスをつくらない */
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_NOCLDWAIT;
    retval = sigaction(SIGCHLD, &sa, NULL);
    if (retval < 0)
        cut_notify("sigaction=%d", retval);

    /* リダイレクト */
    fd = open("/dev/null", O_RDWR, 0);
    if (fd < 0) {
        cut_notify("open=%d", fd);
    } else {
        retval = dup2(fd, STDERR_FILENO);
        if (retval < 0)
            cut_notify("dup2=%d", retval);
        if (fd > 2) {
            retval = close(fd);
            if (retval < 0)
                cut_notify("close=%d", retval);
        }
    }

    /* ソケットファイル文字列設定 */
    tmpret = tmpnam(sockfile);
    if (!tmpret)
        cut_error("tmpnam=%p(%d)", tmpret, errno);

    /* sockaddr_un構造体の設定 */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    (void)strncpy(addr.sun_path, sockfile, sizeof(addr.sun_path));
    addrlen = sizeof(addr.sun_family) + strlen(addr.sun_path);
}

/**
 * set_hostname() 関数テスト
 *
 * @return なし
 */
void
test_set_hostname(void)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    struct in_addr addr;       /* IPアドレス情報構造体 */
    int retval = 0;            /* 戻り値 */

    /* テストデータ */
    const char *host[] = { "localhost", "127.0.0.1" }; /* ホスト文字列 */
    const char *ipaddr = "127.0.0.1";                  /* アドレス */
    const char nohost[] = "nohostxhlkjiherlgfsd";      /* エラー用データ */

    /* 正常系 */
    uint i;
    for (i = 0; i < NELEMS(host); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));
        (void)memset(&addr, 0, sizeof(struct in_addr));

        retval = set_hostname(&server, &addr, host[i]);

        cut_assert_equal_string(ipaddr, inet_ntoa(addr),
                                cut_message("expected=%s, actual=%s",
                                            ipaddr, inet_ntoa(addr)));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
    }

    /* 異常系 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));
    (void)memset(&addr, 0, sizeof(struct in_addr));

    retval = set_hostname(&server, &addr, nohost);

    cut_assert_equal_int(EX_NG, retval,
                         cut_message("return value"));
}

/**
 * set_port() 関数テスト
 *
 * @return なし
 */
void
test_set_port(void)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    int retval = 0;            /* 戻り値 */

    /* テストデータ */
    const char *port[] = { "1", "65534", "ftp" }; /* ポート文字列 */
    const uint32_t portno[] = { 1, 65534, 21 };   /* ポート番号 */
    const char *err_port[] = { "0", "65535", "noservice" }; /* エラー */

    /* 正常系 */
    uint i;
    for (i = 0; i < NELEMS(port); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));

        retval = set_port(&server, port[i]);
        cut_assert_equal_uint((uint)portno[i],
                              (uint)ntohs((uint16_t)server.sin_port),
                              cut_message("expected=%u, actual=%u",
                                          portno[i], ntohs(server.sin_port)));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
    }

    /* 異常系 */
    for (i = 0; i < NELEMS(err_port); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));

        retval = set_port(&server, err_port[i]);
        cut_assert_equal_int(EX_NG, retval,
                             cut_message("return value"));
    }
}

/**
 * set_block() 関数テスト
 *
 * @return なし
 */
void
test_set_block(void)
{
    int fd = 0;     /* ディスクリプタ */
    int retval = 0; /* テスト関数戻り値 */
    int flags = 0;  /* fcntl戻り値 */

    fd = open("/dev/null", O_RDWR, 0);
    if (fd < 0) {
        cut_notify("open=%d", fd);
        return;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        cut_notify("fcntl=%d", flags);
        return;
    }
    dbglog("flags=%d", flags);

    /* 正常系 */
    /* ノンブロッキング */
    retval = set_block(fd, NONBLOCK);

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        cut_notify("fcntl=%d", flags);
        return;
    }

    cut_assert_equal_int(O_NONBLOCK, flags & O_NONBLOCK);
    cut_assert_equal_int(EX_OK, retval,
                         cut_message("return value"));

    /* ブロッキング */
    retval = set_block(fd, BLOCKING);

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        cut_notify("fcntl=%d", flags);
        return;
    }

    cut_assert_equal_int(0, flags & O_NONBLOCK);
    cut_assert_equal_int(EX_OK, retval,
                         cut_message("return value"));

    /* 異常系 */
    retval = set_block(fd, 2);
    cut_assert_equal_int(EX_NG, retval,
                         cut_message("return value"));
}

/**
 * send_data() 関数テスト
 *
 * @return なし
 */
void
test_send_data(void)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* バイト数 */
    pid_t pid = 0;     /* プロセスID */
    int ssock = -1;    /* サーバソケット */
    int csock = -1;    /* クライアントソケット */
    char servbuf[sizeof(sendbuf)] = {0}; /* サーバ受信用バッファ */

    ssock = unix_sock_server();
    if (ssock < 0)
        return;

    pid = fork();
    if (pid < 0) { /* エラー */
        cut_error("fork=%d(%d)", pid, errno);
        return;
    }

    if (pid == 0) { /* 子プロセス */
        server_proc(ssock, servbuf, sizeof(servbuf));
        exit(EXIT_SUCCESS);
    } else { /* 親プロセス */
        dbglog("parent");
        csock = unix_sock_client();
        if (csock < 0)
            return;

        length = sizeof(sendbuf);
        retval = send_data(csock, sendbuf, &length);
        dbglog("send_data=%d, %s", retval, sendbuf);
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
        retval = close(csock);
        if (retval < 0)
            cut_notify("close=%d(%d)", retval, errno);
    }

    retval = unlink(sockfile);
    if (retval < 0)
        outlog("unlink=%d(%d)", retval, errno);
}

/**
 * recv_data() 関数テスト
 *
 * @return なし
 */
void
test_recv_data(void)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* バイト数 */
    ssize_t len = 0;   /* 送信されたバイト数 */
    pid_t pid = 0;     /* プロセスID */
    int ssock = -1;    /* サーバソケット */
    int csock = -1;    /* クライアントソケット */
    char readbuf[sizeof(sendbuf)] = {0}; /* クライアント受信用バッファ */
    char servbuf[sizeof(command)] = {0}; /* サーバ受信用バッファ */

    ssock = unix_sock_server();
    if (ssock < 0)
        return;

    pid = fork();
    if (pid < 0) {
        cut_error("fork=%d(%d)", pid, errno);
        return;
    }

    if (pid == 0) { /* 子プロセス */
        server_proc(ssock, servbuf, sizeof(servbuf));
        exit(EXIT_SUCCESS);
    } else { /* 親プロセス */
        dbglog("parent");
        csock = unix_sock_client();
        if (csock < 0)
            return;

        len = writen(csock, command, sizeof(command));
        if (len < 0) {
            cut_error("writen=%d(%d)", len, errno);
            return;
        }

        length = sizeof(readbuf);
        retval = recv_data(csock, readbuf, &length);

        cut_assert_equal_memory(sendbuf, sizeof(sendbuf),
                                readbuf, sizeof(readbuf),
                                cut_message("%s==%s", sendbuf, readbuf));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
        retval = close(csock);
        if (retval < 0)
            cut_notify("close=%d(%d)", retval, errno);
    }

    retval = unlink(sockfile);
    if (retval < 0)
        outlog("unlink=%d(%d)", retval, errno);
}

/**
 * recv_data_new() 関数テスト
 *
 * @return なし
 */
void
test_recv_data_new(void)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* バイト数 */
    ssize_t len = 0;   /* 送信されたバイト数 */
    pid_t pid = 0;     /* プロセスID */
    int ssock = -1;    /* サーバソケット */
    int csock = -1;    /* クライアントソケット */
    char *readbuf;     /* クライアント受信用ポインタ */
    char servbuf[sizeof(command)] = {0}; /* サーバ受信用バッファ */

    ssock = unix_sock_server();
    if (ssock < 0)
        return;

    pid = fork();
    if (pid < 0) {
        cut_error("fork=%d(%d)", pid, errno);
        return;
    }

    if (pid == 0) { /* 子プロセス */
        server_proc(ssock, servbuf, sizeof(servbuf));
        exit(EXIT_SUCCESS);
    } else { /* 親プロセス */
        dbglog("parent");
        csock = unix_sock_client();
        if (csock < 0)
            return;

        len = writen(csock, command, sizeof(command));
        if (len < 0) {
            cut_error("writen=%d(%d)", len, errno);
            return;
        }

        length = sizeof(sendbuf);
        readbuf = recv_data_new(csock, &length);

        cut_assert_equal_memory(sendbuf, sizeof(sendbuf),
                                readbuf, length,
                                cut_message("%s==%s", sendbuf, readbuf));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
        retval = close(csock);
        if (retval < 0)
            cut_notify("close=%d(%d)", retval, errno);
        if (readbuf)
            free(readbuf);
    }

    retval = unlink(sockfile);
    if (retval < 0)
        outlog("unlink=%d(%d)", retval, errno);
}

/**
 * close_sock() 関数テスト
 *
 * @return なし
 */
void
test_close_sock(void)
{

    int retval = 0;    /* 戻り値 */
    ssize_t len = 0;   /* 送信されたバイト数 */
    pid_t pid = 0;     /* プロセスID */
    int ssock = -1;    /* サーバソケット */
    int csock = -1;    /* クライアントソケット */
    char servbuf[sizeof(command)] = {0}; /* サーバ受信用バッファ */

    ssock = unix_sock_server();
    if (ssock < 0)
        return;

    pid = fork();
    if (pid < 0) {
        cut_error("fork=%d(%d)", pid, errno);
        return;
    }

    if (pid == 0) { /* 子プロセス */
        server_proc(ssock, servbuf, sizeof(servbuf));
        exit(EXIT_SUCCESS);
    } else { /* 親プロセス */
        dbglog("parent");
        csock = unix_sock_client();
        if (csock < 0)
            return;

        len = writen(csock, sendbuf, sizeof(sendbuf));
        if (len < 0) {
            cut_error("writen=%d(%d)", len, errno);
            return;
        }

        /* 正常系 */
        retval = close_sock(&csock);
        cut_assert_equal_int(-1, csock);
        cut_assert_equal_int(EX_OK, retval);
        /* 異常系 */
        retval = close_sock(&csock); /* -1のときはなにもしない */
        cut_assert_equal_int(EX_OK, retval);
    }

    retval = unlink(sockfile);
    if (retval < 0)
        outlog("unlink=%d(%d)", retval, errno);
}

/**
 * サーバプロセス
 *
 * @param[in] sockfd ソケット
 * @param[in] readbuf 受信バッファ
 * @param[in] length バッファ長さ
 * @return なし
 */
static void
server_proc(int sockfd, char *readbuf, size_t length)
{
    socklen_t len = 0; /* sockaddr構造体長さ */
    ssize_t rlen = 0;  /* 受信された長さ */
    ssize_t wlen = 0;  /* 送信された長さ */
    int acc = -1;      /* accept戻り値 */
    int retval = 0;    /* 戻り値 */

    dbglog("start");

    len = addrlen;
    dbglog("accept=%d(%d)", sockfd, errno);
    acc = accept(sockfd, (struct sockaddr *)&addr, &len);
    if (acc < 0) {
        cut_error("accept=%d(%d)", acc, errno);
        return;
    }
    dbglog("accept=%d(%d)", acc, errno);

    rlen = readn(acc, readbuf, length);
    if (rlen < 0) {
        cut_error("readn=%d(%d)", rlen, errno);
        return;
    }

    retval = strncmp(readbuf, command, strlen(command));
    dbglog("strncmp=%d, %s==%s", retval, readbuf, command);
    if (retval == 0) { /* 送信 */
        dbglog("accept=%d(%d)", acc, errno);
        wlen = writen(acc, sendbuf, sizeof(sendbuf));
        if (wlen < 0) {
            cut_error("writen=%d(%d)", wlen, errno);
            return;
        }
    } else {
        retval = memcmp(sendbuf, readbuf, sizeof(sendbuf));
        if (retval < 0) {
            /* 受信されたデータが違う場合, エラー */
            cut_error("memcmp=%d(%d)", retval, errno);
            return;
        }
    }
    retval = close(acc);
    if (retval < 0)
        cut_notify("close=%d(%d)", retval, errno);
}

/**
 * サーバソケット生成
 *
 * @return ソケット
 */
static int
unix_sock_server(void)
{
    int retval = 0;  /* 戻り値 */
    int sockfd = -1; /* ソケット */
    /* ファイルの許可 */
    const mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

    dbglog("start");

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cut_error("socket=%d(%d)", sockfd, errno);
        return EX_ERROR;
    }

    retval = bind(sockfd, (struct sockaddr *)&addr, addrlen);
    if (retval < 0) {
        if (errno == EADDRINUSE)
            cut_error("Address already in use");
        cut_error("bind=%d, sockfd=%d(%d)", retval, sockfd, errno);
        return EX_ERROR;
    }

    retval = listen(sockfd, SOMAXCONN);
    if (retval < 0) {
        cut_error("listen=%d, sockfd=%d(%d)", retval, sockfd, errno);
        return EX_ERROR;
    }

    retval = chmod(sockfile, mode);
    if (retval < 0) {
        cut_error("chmod=%d, sockfd=%d(%d)", retval, sockfd, errno);
        return EX_ERROR;
    }

    return sockfd;
}

/**
 * クライアントソケット生成
 *
 * @return ソケット
 */
static int
unix_sock_client(void)
{
    int retval = 0;  /* 戻り値 */
    int sockfd = -1; /* ソケット */

    dbglog("start");

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cut_error("socket=%d(%d)", sockfd, errno);
        return EX_ERROR;
    }

    retval = connect(sockfd, (struct sockaddr *)&addr, addrlen);
    if (retval < 0) {
        outlog("connect=%d(%d), %s", sockfd, errno, strerror(errno));
        return EX_ERROR;
    }
    dbglog("connect=%d(%d), %s", sockfd, errno, strerror(errno));

    return sockfd;
}


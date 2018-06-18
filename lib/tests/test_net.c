/**
 * @file lib/tests/test_net.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-15 higashi 新規作成
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

#include <stdio.h>     /* tmpnam */
#include <unistd.h>    /* access fork */
#include <fcntl.h>     /* open fcntl */
#include <arpa/inet.h> /* inet_ntoa */
#include <sys/stat.h>  /* chmod */
#include <sys/wait.h>  /* wait waitpid */
#include <errno.h>     /* errno */
#include <signal.h>    /* signal */
#include <cutter.h>    /* cutter library */

#include "def.h"
#include "log.h"
#include "fileio.h"
#include "net.h"

#define BUF_SIZE 2048

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
static char sockfile[L_tmpnam] = {0}; /**< ソケットファイル */
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
 *
 * @return なし
 */
void
cut_startup(void)
{
    set_sig_handler();

    /* ソケットファイル文字列設定 */
    if (!tmpnam(sockfile))
        cut_error("tmpnam(%d)", errno);

    /* sockaddr_un構造体の設定 */
    (void)memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    (void)strncpy(addr.sun_path, sockfile, sizeof(addr.sun_path) - 1);
    addrlen = sizeof(addr.sun_family) + strlen(addr.sun_path);
}

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_setup(void)
{
    (void)memset(sendbuf, 'a', sizeof(sendbuf));
    sendbuf[sizeof(sendbuf) - 1] = '\0';
    sendbuf[sizeof(sendbuf) - 2] = '\n';
}

/**
 * 終了処理
 *
 * @return なし
 */
void
cut_teardown(void)
{
    if (acc != -1) {
        if (close(acc) < 0)
            cut_notify("close: acc=%d(%d)", acc, errno);
        acc = -1;
    }
    if (ssock != -1) {
        if (close(ssock) < 0)
            cut_notify("close: ssock=%d(%d)", ssock, errno);
        ssock = -1;
    }
    if (csock != -1) {
        if (close(csock) < 0)
            cut_notify("close: csock=%d(%d)", csock, errno);
        csock = -1;
    }
    if (fd != -1) {
        if (close(fd) < 0)
            cut_notify("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }

    if (sockfile[0] != '\0') {
        if (!access(sockfile, W_OK)) { /* ファイルが存在する */
            if (unlink(sockfile) < 0)
                cut_notify("unlink: %s(%d)", sockfile, errno);
        }
    }

    if (readnew)
        free(readnew);
    readnew = NULL;
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

        cut_assert_equal_string(ipaddr, inet_ntoa(server.sin_addr),
                                cut_message("expected=%s, actual=%s",
                                            ipaddr,
                                            inet_ntoa(server.sin_addr)));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
    }

    /* 異常系 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));

    retval = set_hostname(&server, nohost);

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
    unsigned int i;
    for (i = 0; i < NELEMS(port); i++) {
        (void)memset(&server, 0, sizeof(struct sockaddr_in));

        retval = set_port(&server, port[i]);
        cut_assert_equal_uint((unsigned int)portno[i],
                              (unsigned int)ntohs((uint16_t)server.sin_port),
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
    retval = set_block(fd, (blockmode)2);
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
    pid_t cpid = 0;    /* 子プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char servbuf[sizeof(sendbuf)]; /* サーバ受信用バッファ */

    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        cut_error("unix_sock_server");
        return;
    }

    cpid = fork();
    if (cpid < 0) { /* エラー */
        cut_error("fork(%d)", errno);
        return;
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
            cut_error("unix_sock_client");
            return;
        }

        /* テスト関数の実行 */
        length = sizeof(sendbuf);
        retval = send_data(csock, sendbuf, &length);
        dbglog("send_data=%d, %s", retval, sendbuf);
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d", WEXITSTATUS(status));
    }

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
    pid_t cpid = 0;    /* プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char readbuf[sizeof(sendbuf)]; /* クライアント受信用バッファ */
    char servbuf[sizeof(command)]; /* サーバ受信用バッファ */

    (void)memset(readbuf, 0, sizeof(readbuf));
    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        cut_error("unix_sock_server");
        return;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
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
            cut_error("unix_sock_client");
            return;
        }

        len = writen(csock, command, sizeof(command));
        if (len < 0) {
            cut_error("writen=%zd(%d)", len, errno);
            return;
        }

        /* テスト関数の実行 */
        length = sizeof(readbuf);
        retval = recv_data(csock, readbuf, &length);

        cut_assert_equal_memory(sendbuf, sizeof(sendbuf),
                                readbuf, sizeof(readbuf),
                                cut_message("%s==%s", sendbuf, readbuf));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d", WEXITSTATUS(status));
    }
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
    pid_t cpid = 0;    /* プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char servbuf[sizeof(command)]; /* サーバ受信用バッファ */

    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        cut_error("unix_sock_server");
        return;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
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
            cut_error("unix_sock_client");
            return;
        }

        len = writen(csock, command, sizeof(command));
        if (len < 0) {
            cut_error("writen=%zd(%d)", len, errno);
            return;
        }

        /* テスト関数の実行 */
        length = sizeof(sendbuf);
        readnew = (char *)recv_data_new(csock, &length);

        cut_assert_equal_memory(sendbuf, sizeof(sendbuf),
                                readnew, length,
                                cut_message("%s==%s", sendbuf, readnew));
        cut_assert_equal_int(EX_OK, retval,
                             cut_message("return value"));
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d", WEXITSTATUS(status));
    }
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
    pid_t cpid = 0;    /* プロセスID */
    pid_t w = 0;       /* wait戻り値 */
    int status = 0;    /* ステイタス */
    char servbuf[sizeof(sendbuf)]; /* サーバ受信用バッファ */

    (void)memset(servbuf, 0, sizeof(servbuf));

    ssock = unix_sock_server();
    if (ssock < 0) {
        cut_error("unix_sock_server");
        return;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
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
            cut_error("unix_sock_client");
            return;
        }

        len = writen(csock, sendbuf, sizeof(sendbuf));
        if (len < 0) {
            cut_error("writen=%zd(%d)", len, errno);
            return;
        }

        /* テスト関数の実行 */
        /* 正常系 */
        retval = close_sock(&csock);
        cut_assert_equal_int(-1, csock);
        cut_assert_equal_int(EX_OK, retval);
        /* -1のときはなにもしない */
        retval = close_sock(&csock);

        cut_assert_equal_int(EX_OK, retval);

        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d", WEXITSTATUS(status));
    }
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
        cut_notify("socket(%d)", errno);
        return EX_NG;
    }

    retval = bind(sockfd, (struct sockaddr *)&addr, addrlen);
    if (retval < 0) {
        if (errno == EADDRINUSE)
            cut_notify("Address already in use");
        cut_notify("bind: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    retval = listen(sockfd, SOMAXCONN);
    if (retval < 0) {
        cut_notify("listen sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    retval = chmod(sockfile, mode);
    if (retval < 0) {
        cut_notify("chmod: sockfd=%d(%d)", sockfd, errno);
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
        cut_notify("socket(%d)", errno);
        return EX_NG;
    }

    retval = connect(sockfd, (struct sockaddr *)&addr, addrlen);
    if (retval < 0) {
        cut_notify("connect=%d(%d)", sockfd, errno);
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


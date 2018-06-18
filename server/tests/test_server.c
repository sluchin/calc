/**
 * @file server/tests/test_server.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-24 higashi 新規作成
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

#include <stdio.h>      /* setvbuf stdin stdout */
#include <string.h>     /* memset */
#include <unistd.h>     /* pipe fork */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* sockopt etc... */
#include <arpa/inet.h>  /* ntohl */
#include <sys/wait.h>   /* wait */
#include <signal.h>     /* signal */
#include <errno.h>      /* errno */
#include <cutter.h>     /* cutter library */

#include "def.h"
#include "log.h"
#include "net.h"
#include "data.h"
#include "fileio.h"
#include "memfree.h"
#include "server.h"

#define BUF_SIZE    30  /**< バッファサイズ */
#define MAX_THREADS  5  /**< スレッド数 */
/* MAX_THREADS 1013 まで
 * 1014 からテストエラー
 */

/** スレッドデータ構造体 */
struct send_data {
    unsigned char sdata[BUF_SIZE];    /**< 送信データ */
    unsigned char rdata[BUF_SIZE];    /**< 受信データ */
    unsigned char expected[BUF_SIZE]; /**< 期待される文字列 */
    size_t len;                       /**< 送信データ長 */
};

/* プロトタイプ */
/** set_port_string() 関数テスト */
void test_set_port_string(void);
/** server_sock() 関数テスト */
void test_server_sock(void);
/** server_loop() 関数テスト */
void test_server_loop(void);
/** server_proc() 関数テスト */
void test_server_proc(void);

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
/** 送信 */
static int send_client(int sockfd, unsigned char *sbuf, size_t length);
/** 受信 */
static int recv_client(int sockfd, unsigned char *rbuf);
/** ソケット生成 */
static int inet_sock_client(void);
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

    /* バッファリングしない */
    if (setvbuf(stdin, NULL, _IONBF, 0))
        cut_notify("setvbuf: stdin(%d)", errno);
    if (setvbuf(stdout, NULL, _IONBF, 0))
        cut_notify("setvbuf: stdout(%d)", errno);

    (void)memset(&server, 0, sizeof(testserver));
    test_init_server(&server);

    /* リダイレクト */
    redirect(STDERR_FILENO, "/dev/null");
}

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_setup(void)
{
    (void)memset(readbuf, 0, sizeof(readbuf));
}

/**
 * 終了処理
 *
 * @return なし
 */
void
cut_teardown(void)
{
    close_sock(&ssock);
    close_sock(&csock);
}

/**
 * test_set_port_string() 関数テスト
 *
 * @return なし
 */
void
test_set_port_string(void)
{
    int retval = 0;                  /* 戻り値 */
    const char *err_port = "123456"; /* エラー用 */

    /* 正常系 */
    retval = set_port_string(port);
    cut_assert_equal_int(EX_OK, retval);
    /* 異常系 */
    retval = set_port_string(err_port);
    cut_assert_equal_int(EX_NG, retval);
}

/**
 * test_server_sock() 関数テスト
 *
 * @return なし
 */
void
test_server_sock(void)
{
    dbglog("start");

    if (set_port_string(port) < 0)
        cut_error("set_port_string");
    ssock = server_sock();
    dbglog("server_sock=%d", ssock);

    cut_assert_not_equal_int(EX_NG, ssock);

    csock = inet_sock_client();
    if (csock < 0)
        cut_error("inet_sock_client");

}

/**
 * test_server_loop() 関数テスト
 *
 * @return なし
 */
void
test_server_loop(void)
{
    pid_t cpid = 0; /* 子プロセスID */
    pid_t w = 0;    /* wait戻り値 */
    int status = 0; /* wait引数 */
    int retval = 0; /* 戻り値 */
    int count = 1;  /* ループカウント */

    if (set_port_string(port) < 0)
        cut_error("set_port_string");
    ssock = server_sock();

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
    }

    if (cpid == 0) {
        dbglog("child");

        count = 2;
        g_sig_handled = 1;
        while (count--)
            server_loop(ssock);
        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        csock = inet_sock_client();
        if (csock < 0)
            return;

        /* 送信 */
        retval = send_client(csock, expr, sizeof(expr));
        if (retval < 0) {
            cut_error("send_client: csock=%d(%d)", csock, errno);
            return;
        }

        /* 受信 */
        retval = recv_client(csock, readbuf);
        if (retval < 0) {
            cut_error("recv_client: csock=%d(%d)", csock, errno);
            return;
        }

        cut_assert_equal_string((char *)expected, (char *)readbuf);

        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
    }
}

/**
 * test_server_proc() 関数テスト
 *
 * @return なし
 */
void
test_server_proc(void)
{
    pid_t cpid = 0;         /* 子プロセスID */
    pid_t w = 0;            /* wait戻り値 */
    int status = 0;         /* wait引数 */
    int retval = 0;         /* 戻り値 */
    thread_data *dt = NULL; /* ソケット情報構造体 */
    void *servret = NULL;   /* テスト関数戻り値 */

    if (set_port_string(port) < 0)
        cut_error("set_port_string");
    ssock = server_sock();

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
    }

    if (cpid == 0) {
        dbglog("child");

        dt = (thread_data *)malloc(sizeof(thread_data));
        if (!dt) {
            outlog("malloc: size=%zu", sizeof(thread_data));
            exit(EXIT_FAILURE);
        }
        (void)memset(dt, 0, sizeof(thread_data));

        dt->len = (socklen_t)sizeof(dt->addr);
        dt->sock = accept(ssock, (struct sockaddr *)&dt->addr, &dt->len);
        if (dt->sock < 0) {
            outlog("accept: ssock=%d", ssock);
            memfree((void **)&dt, NULL);
            exit(EXIT_FAILURE);
        }
        g_sig_handled = 1;

        /* テスト関数実行 */
        servret = server.server_proc(dt);
        if (servret) {
            outlog("server_proc");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);

    } else {
        dbglog("parent: cpid=%d", (int)cpid);

        csock = inet_sock_client();
        if (csock < 0)
            return;

        /* 送信 */
        retval = send_client(csock, expr, sizeof(expr));
        if (retval < 0) {
            cut_error("send_client: csock=%d(%d)", csock, errno);
            return;
        }

        /* 受信 */
        retval = recv_client(csock, readbuf);
        if (retval < 0) {
            cut_error("recv_client: csock=%d(%d)", csock, errno);
            return;
        }

        cut_assert_equal_string((char *)expected, (char *)readbuf);

        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("child failed");
    }
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
send_client(int sockfd, unsigned char *sbuf, size_t length)
{
    struct client_data *cdata = NULL; /* 送信データ構造体 */
    ssize_t slen = 0;                 /* 送信データバイト数 */
    int retval = 0;                   /* 戻り値 */

    dbglog("start");

    /* データ設定 */
    slen = set_client_data(&cdata, sbuf, length);
    if (slen < 0) {
        cut_notify("set_server_data=%zd(%d)", slen, errno);
        return EX_NG;
    }

    /* 送信 */
    retval = send_data(sockfd, cdata, (size_t *)&slen);
    if (retval < 0) {
        cut_notify("send_data: slen=%zd(%d)", slen, errno);
        memfree((void **)&cdata, NULL);
        return EX_NG;
    }
    memfree((void **)&cdata, NULL);
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
    size_t length = 0; /* バイト数 */
    struct header hd;  /* ヘッダ構造体 */
    int retval = 0;    /* 戻り値 */

    dbglog("start");

    /* ヘッダ受信 */
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);

    retval = recv_data(sockfd, &hd, &length);
    if (retval < 0) {
        cut_notify("recv_data: length=%zu(%d)", length, errno);
        return EX_NG;
    }
    length = (size_t)ntohl((uint32_t)hd.length);

    /* 受信 */
    retval = recv_data(sockfd, rbuf, &length);
    if (retval < 0) {
        cut_notify("recv_data: length=%zu(%d)", length, errno);
        return EX_NG;
    }
    return EX_OK;
}

/**
 * ソケット作成
 *
 * @return なし
 */
static int
inet_sock_client(void)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    int sockfd = 0;            /* ソケット */
    int retval = 0;            /* 戻り値 */

    dbglog("start");

    /* 初期化 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;

    if (set_hostname(&server, hostname) < 0)
        return EX_NG;
    if (set_port(&server, port) < 0)
        return EX_NG;

    /* ソケット生成 */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        outlog("sock=%d", sockfd);
        return EX_NG;
    }

    /* コネクト */
    retval = connect(sockfd, (struct sockaddr *)&server,
                     sizeof(struct sockaddr_in));
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
 *
 * @return なし
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
    if (signal(SIGTERM, SIG_IGN) < 0)
        cut_notify("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) < 0)
        cut_notify("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) < 0)
        cut_notify("SIGHUP");
    if (signal(SIGALRM, SIG_IGN) < 0)
        cut_notify("SIGALRM");
}


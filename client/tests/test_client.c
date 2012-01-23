/**
 * @file client/tests/test_client.c
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

#include <stdio.h>      /* snprintf */
#include <string.h>     /* memset strndup */
#include <unistd.h>     /* close pipe fork */
#include <fcntl.h>      /* open */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* setsockopt */
#include <arpa/inet.h>  /* inet_addr ntohl */
#include <sys/select.h> /* select */
#include <sys/wait.h>   /* wait */
#include <signal.h>     /* signal sigaction */
#include <errno.h>      /* errno */
#include <cutter.h>     /* cutter library */

#include "def.h"
#include "log.h"
#include "net.h"
#include "data.h"
#include "fileio.h"
#include "memfree.h"
#include "client.h"

#define BUF_SIZE 4100 /**< バッファサイズ */

/* 内部変数 */
static testclient client;                  /**< 関数構造体 */
static char port[] = "12345";              /**< ポート番号 */
static const char *hostname = "localhost"; /**< ホスト名 */
static struct sockaddr_in addr;            /**< ソケットアドレス情報構造体 */
static uchar readbuf[BUF_SIZE];            /**< 受信バッファ */
static uchar sendbuf[BUF_SIZE];            /**< 送信データ */
static int ssock = -1;                     /**< サーバソケット */
static int csock = -1;                     /**< クライアントソケット */
static int acc = -1;                       /**< アクセプト */
static int pfd1[] = { -1, -1 };            /**< パイプ1 */
static int pfd2[] = { -1, -1 };            /**< パイプ2 */
static const int CHILD_FAILED = 255;       /**< 子プロセス失敗 */

/* プロトタイプ */
/** connect_sock() 関数テスト */
void test_connect_sock(void);
/** client_loop() 関数テスト */
void test_client_loop(void);
/** read_stdin() 関数テスト */
void test_read_stdin(void);
/** send_sock() 関数テスト */
void test_send_sock(void);
/** read_sock() 関数テスト */
void test_read_sock(void);

/* 内部関数 */
/** send_sock() 関数実行 */
static int exec_send_sock(uchar *sbuf, size_t length);
/** アクセプト */
static int accept_server(int sockfd);
/** 受信 */
static int recv_server(int sockfd, uchar *rbuf);
/** 送信 */
static int send_server(int sockfd, uchar *sbuf, size_t length);
/** ソケット生成 */
static int inet_sock_server(void);
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
void
cut_setup(void)
{
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
void
cut_teardown(void)
{
    int retval = 0; /* 戻り値 */

    close_fd(&pfd1[PIPE_R], &pfd1[PIPE_W],
             &pfd2[PIPE_R], &pfd2[PIPE_W], NULL);

    if (ssock != -1) {
        retval = shutdown(ssock, SHUT_RDWR);
        if (retval < 0)
            cut_notify("shutdown=%d(%d)", ssock, errno);
    }

    close_sock(&acc);
    close_sock(&ssock);
    close_sock(&csock);
}

/**
 * test_connect_sock() 関数テスト
 *
 * @return なし
 */
void
test_connect_sock(void)
{
    dbglog("start");

    ssock = inet_sock_server();
    if (ssock < 0) {
        cut_error("inet_sock_server");
        return;
    }

    csock = connect_sock(hostname, port);
    dbglog("connect_sock=%d", csock);

    cut_assert_not_equal_int(EX_NG, csock);
}

/**
 * test_client_loop() 関数テスト
 *
 * @return なし
 */
void
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
        cut_error("pipe(%d)", errno);
        return;
    }

    retval = pipe(pfd2);
    if (retval < 0) {
        cut_error("pipe(%d)", errno);
        return;
    }

    ssock = inet_sock_server();
    if (ssock < 0) {
        cut_error("inet_sock_server");
        return;
    }

    oldfd = dup(STDOUT_FILENO);
    if (oldfd < 0)
        cut_notify("dup(%d)", errno);
    redirect(STDOUT_FILENO, "/dev/null");

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        /* コネクト */
        csock = connect_sock(hostname, port);
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

        g_sig_handled = 1;
        st = client_loop(csock);

        close_sock(&csock);
        exit(st);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        /* 標準入力 */
        retval = pipe_fd2(&pfd1[PIPE_R], &pfd1[PIPE_W], STDIN_FILENO);
        if (retval < 0) {
            cut_error("pipe_fd2(%d)", errno);
            return;
        }

        /* 標準入力に送信 */
        sendlen = sizeof(sendbuf);
        wlen = write(STDIN_FILENO, (char *)sendbuf, sendlen);
        if (wlen < 0) {
            cut_error("write=%zd(%d)", wlen, errno);
            return;
        }
        dbglog("write=%zd", wlen);

        /* 受信待ち */
        acc = accept_server(ssock);
        if (acc < 0)
            return;

        /* 受信 */
        retval = recv_server(acc, readbuf);
        if (retval < 0) {
            cut_error("recv_server: acc=%d(%d)", acc, errno);
            return;
        }

        /* 改行削除 */
        if (sendbuf[strlen((char *)sendbuf) - 1] == '\n')
            sendbuf[strlen((char *)sendbuf) - 1] = '\0';

        cut_assert_equal_memory(sendbuf, sendlen,
                                readbuf, sendlen);

        /* 送信 */
        sendlen = strlen((char *)readbuf) + 1;
        retval = send_server(acc, readbuf, sendlen);
        if (retval < 0) {
            cut_error("send_server: acc=%d(%d)", acc, errno);
            return;
        }

        /* 標準出力 */
        retval = pipe_fd2(&pfd2[PIPE_W], &pfd2[PIPE_R], STDOUT_FILENO);
        if (retval < 0) {
            cut_error("pipe_fd2(%d)", errno);
            return;
        }

        /* 標準出力から受信 */
        (void)memset(readbuf, 0, sizeof(readbuf));
        rlen = readn(STDOUT_FILENO, (char *)readbuf, sendlen - 1);
        if (rlen < 0) {
            cut_error("read=%zd(%d)", rlen, errno);
            return;
        }
        dbglog("readbuf=%s", readbuf);

        /* 標準出力を元に戻す */
        if (dup2(oldfd, STDOUT_FILENO) < 0)
            cut_notify("dup2(%d)", errno);

        cut_assert_equal_memory(sendbuf, sendlen,
                                readbuf, sendlen);
        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) == CHILD_FAILED)
            cut_error("status=%d(%d)", WEXITSTATUS(status), errno);
    }
}

/**
 * test_send_sock() 関数テスト
 *
 * @return なし
 */
void
test_send_sock(void)
{
    int retval = 0;          /* 戻り値 */
    uchar estr[] = "exit\n"; /* exit文字列 */
    uchar qstr[] = "quit\n"; /* quit文字列 */

    dbglog("start");

    retval = exec_send_sock(sendbuf, sizeof(sendbuf));
    cut_assert_equal_int(EX_SUCCESS, retval);
    cut_teardown();

    retval = exec_send_sock(estr, sizeof(estr));
    cut_assert_equal_int(EX_QUIT, retval);
    cut_teardown();

    retval = exec_send_sock(qstr, sizeof(qstr));
    cut_assert_equal_int(EX_QUIT, retval);
    cut_teardown();
}

/**
 * test_read_sock() 関数テスト
 *
 * @return なし
 */
void
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
        cut_error("pipe(%d)", errno);
        return;
    }

    ssock = inet_sock_server();
    if (ssock < 0) {
        cut_error("inet_sock_server");
        return;
    }

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return;
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        csock = connect_sock(hostname, port);
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
            cut_error("pipe_fd2(%d)", errno);
            return;
        }

        /* 受信待ち */
        acc = accept_server(ssock);
        retval = send_server(acc, sendbuf, sizeof(sendbuf));
        if (retval < 0) {
            cut_error("send_server: acc=%d(%d)", acc, errno);
            return;
        }

        /* 標準出力から受信 */
        rlen = readn(STDOUT_FILENO, (char *)readbuf, sizeof(sendbuf));
        if (rlen < 0) {
            cut_error("read=%zd(%d)", rlen, errno);
            return;
        }

        /* 標準出力を元に戻す */
        if (dup2(oldfd, STDOUT_FILENO) < 0)
            cut_notify("dup2(%d)", errno);

        cut_assert_equal_memory(sendbuf, strlen((char *)sendbuf),
                                readbuf, strlen((char *)sendbuf));
        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status))
            cut_error("status=%d(%d)", WEXITSTATUS(status), errno);
    }
}

/**
 * send_sock() 関数実行
 *
 * @return なし
 */
static int
exec_send_sock(uchar *sbuf, size_t length)
{
    pid_t cpid = 0;            /* プロセスID */
    pid_t w = 0;               /* wait戻り値 */
    int status = 0;            /* wait引数 */
    ssize_t wlen = 0;          /* write戻り値 */
    int oldfd = 0;             /* 退避用 */
    int retval = 0;            /* 戻り値 */
    uchar rbuf[length];        /* 受信バッファ */
    st_client st = EX_SUCCESS; /* ステータス */

    dbglog("start");

    retval = pipe(pfd1);
    if (retval < 0) {
        cut_error("pipe(%d)", errno);
        return EX_NG;
    }

    ssock = inet_sock_server();
    if (ssock < 0) {
        cut_error("inet_sock_server");
        return EX_NG;
    }

    oldfd = dup(STDOUT_FILENO);
    if (oldfd < 0)
        cut_notify("dup(%d)", errno);
    redirect(STDOUT_FILENO, "/dev/null");

    cpid = fork();
    if (cpid < 0) {
        cut_error("fork(%d)", errno);
        return EX_NG;
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        csock = connect_sock(hostname, port);
        if (csock < 0) {
            cut_error("pipe_fd2(%d)", errno);
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
            cut_error("pipe_fd2(%d)", errno);
            return EX_NG;
        }

        /* 標準入力に送信 */
        wlen = writen(STDIN_FILENO, (char *)sbuf, length);
        if (wlen < 0) {
            cut_error("write=%zd(%d)", wlen, errno);
            return EX_NG;
        }
        dbglog("write=%zd", wlen);

        if (strcmp((char *)sbuf, "quit\n") &&
            strcmp((char *)sbuf, "exit\n")) {

            /* 受信待ち */
            acc = accept_server(ssock);
            if (acc < 0) {
                cut_error("accept(%d)", errno);
                return EX_NG;
            }

            /* 受信 */
            retval = recv_server(acc, rbuf);
            if (retval < 0) {
                cut_error("recv_server: acc=%d(%d)", acc, errno);
                return EX_NG;
            }

            /* 改行削除 */
            if (sbuf[strlen((char *)sbuf) - 1] == '\n')
                sbuf[strlen((char *)sbuf) - 1] = '\0';

            cut_assert_equal_string((char *)sbuf, (char *)rbuf);
        }

        /* 標準出力を元に戻す */
        if (dup2(oldfd, STDOUT_FILENO) < 0)
            cut_notify("dup2(%d)", errno);

        w = wait(&status);
        if (w < 0)
            cut_notify("wait(%d)", errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) == CHILD_FAILED)
            cut_error("child failed");
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
            cut_notify("select=%d", ready);
            break;
        } else if (ready) {
            if (FD_ISSET(sockfd, &rfds)) {
                /* アクセプト */
                addrlen = (socklen_t)sizeof(struct sockaddr_in);
                accfd = accept(sockfd, (struct sockaddr *)&addr, &addrlen);
                if (accfd < 0) {
                    cut_notify("accept=%d(%d)", accfd, errno);
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
recv_server(int sockfd, uchar *rbuf)
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
 * 送信
 *
 * @param[in] sockfd ソケット
 * @param[in] sbuf 送信バッファ
 * @param[in] length バイト数
 * @retval EX_NG エラー
 */
static int
send_server(int sockfd, uchar *sbuf, size_t length)
{
    struct server_data *sdata = NULL; /* 送信データ構造体 */
    ssize_t slen = 0;                 /* 送信データバイト数 */
    int retval = 0;                   /* 戻り値 */

    dbglog("start");

    /* データ設定 */
    slen = set_server_data(&sdata, sbuf, length);
    if (slen < 0) {
        cut_notify("set_server_data=%zd(%d)", slen, errno);
        return EX_NG;
    }

    /* 送信 */
    retval = send_data(sockfd, sdata, (size_t *)&slen);
    if (retval < 0) {
        cut_notify("send_data: slen=%zd(%d)", slen, errno);
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
        cut_notify("socket(%d)", errno);
        return EX_NG;
    }

    /* ソケットオプション */
    int optval = 1; /* 二値オプション有効 */
    retval = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                        &optval, (socklen_t)sizeof(int));
    if (retval < 0) {
        cut_notify("setsockopt: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    /* ソケットにアドレスを指定 */
    retval = bind(sockfd, (struct sockaddr *)&addr,
                  (socklen_t)sizeof(addr));
    if (retval < 0) {
        if (errno == EADDRINUSE)
            cut_notify("Address already in use\n");
        cut_notify("bind: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    /* アクセスバックログの指定 */
    retval = listen(sockfd, SOMAXCONN);
    if (retval < 0) {
        cut_notify("listen: sockfd=%d(%d)", sockfd, errno);
        return EX_NG;
    }

    /* ノンブロッキングモードに設定 */
    retval = set_block(sockfd, NONBLOCK);
    if (retval < 0) {
        cut_notify("set_block: sockfd=%d", sockfd);
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


/**
 * @file lib/log.c
 * @brief ログ出力
 *
 * @author higashi
 * @date 2010-06-22 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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

#include <stdio.h>     /* fprintf vsprintf */
#include <string.h>    /* memset */
#include <stdlib.h>    /* exit */
#include <stdarg.h>    /* va_list va_start va_end */
#include <errno.h>     /* errno */
#include <time.h>      /* struct tm */
#include <sys/time.h>  /* struct timeval */
#include <sys/types.h> /* getpid */
#include <unistd.h>    /* getpid gethostname */
#include <ctype.h>     /* isprint */
#include <pthread.h>   /* pthread_self */
#include <execinfo.h>  /* backtrace */

#include "def.h"
#include "log.h"

/* 内部変数 */
#define MAX_HOST_SIZE  25 /**< 最大ホスト文字列サイズ */
#define MAX_MES_SIZE  256 /**< 最大メッセージサイズ */

/* 外部変数 */
char *progname = NULL; /**< プログラム名 */

/**
 * シスログ出力
 *
 * @param[in] level ログレベル
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void system_log(const int level,
                const char *pname,
                const char *fname,
                const int line,
                const char *func,
                const char *format, ...)
{
    int errsv = errno;                /* errno退避 */
    int retval = 0;                   /* 戻り値 */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */
    pthread_t tid = 0;                /* スレッドID */
    /* スレッドID用バッファ
     * 64bit ULONG_MAX: 18446744073709551615UL
     * 32bit ULONG_MAX: 4294967295UL */
    char t_buf[sizeof(", tid=18446744073709551615")] = {0};

    /* シスログオープン */
    openlog(pname, SYS_OPT, SYS_FACILITY);

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(level, "%s[%d]: vsnprintf=%d(%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    tid = pthread_self();
    if (tid)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu",
                       (unsigned long int)tid);

    syslog(level, "ppid=%d%s: %s[%d]: %s(%s): %m(%d)",
           getppid(), tid ? t_buf : "", fname, line, func, message, errsv);

    /* シスログクローズ */
    closelog();
    errno = 0; /* errno初期化 */
}

/**
 * シスログ出力(デバッグ用)
 *
 * @param[in] level ログレベル
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void system_dbg_log(const int level,
                    const char *pname,
                    const char *fname,
                    const int line,
                    const char *func,
                    const char *format, ...)
{
    int errsv = errno;                /* errno退避 */
    int retval = 0;                   /* 戻り値 */
    struct tm ts;                     /* tm構造体 */
    struct tm *tsp;                   /* localtime_r戻り値 */
    struct timeval tv;                /* timeval構造体 */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    char d_buf[sizeof("00")] = {0};   /* 秒格納用バッファ */
    va_list ap;                       /* va_list */
    pthread_t tid = 0;                /* スレッドID */
    /* スレッドID用バッファ
     * 64bit ULONG_MAX: 18446744073709551615UL
     * 32bit ULONG_MAX: 4294967295UL */
    char t_buf[sizeof(", tid=18446744073709551615")] = {0};

    /* シスログオープン */
    openlog(pname, SYS_OPT, SYS_FACILITY);

    retval = gettimeofday (&tv, NULL);
    if (retval < 0) {
        syslog(level, "%s[%d]: gettimeofday=%d(%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    tsp = localtime_r(&tv.tv_sec, &ts);
    if (!tsp) {
        syslog(level, "%s[%d]: localtime=%p(%d)",
               __FILE__, __LINE__, tsp, errno);
        return;
    }

    retval = strftime(d_buf, sizeof(d_buf), "%S", &ts);
    if (!retval) {
        syslog(level, "%s[%d] strftime=%d(%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(level, "%s[%d]: vsnprintf=%d(%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    tid = pthread_self();
    if (tid)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu",
                       (unsigned long int)tid);

    syslog(level, "ppid=%d%s: %s.%ld: %s[%d]: %s(%s): %m(%d)",
           getppid(), tid ? t_buf : "",
           d_buf, tv.tv_usec, fname, line, func, message, errsv);

    /* シスログクローズ */
    closelog();
    errno = 0;  /* errno初期化 */
}

/**
 * 標準エラー出力にログ出力
 *
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void stderr_log(const char *pname,
                const char *fname,
                const int line,
                const char *func,
                const char *format, ...)
{
    FILE *fp = stderr;   /* 標準エラー出力 */
    int errsv = errno;   /* errno退避 */
    int retval = 0;      /* 戻り値 */
    char *ascret = NULL; /* asctime戻り値 */
    struct tm ts;        /* tm構造体 */
    struct tm *tsp;      /* localtime_rの戻り値 */
    struct timeval tv;   /* timeval構造体 */
    va_list ap;          /* va_list */
    char d_buf[sizeof("xxx xxx 00 00:00:00 1900\n")] = {0}; /* 時刻 */
    char h_buf[MAX_HOST_SIZE] = {0};                        /* ホスト */
    pthread_t tid = 0;                                      /* スレッドID */
    /* スレッドID用バッファ
     * 64bit ULONG_MAX: 18446744073709551615UL
     * 32bit ULONG_MAX: 4294967295UL */
    char t_buf[sizeof(", tid=18446744073709551615")] = {0};
    char dammy[sizeof("xxx")];     /* 曜日 */
    char mon[sizeof("xxx")];       /* 月 */
    char day[sizeof("xxx")];       /* 日 */
    char hour[sizeof("00:00:00")]; /* 時間 */

    retval = gettimeofday(&tv, NULL);
    if (retval < 0) {
        (void)fprintf(stderr, "%s[%d]: gettimeofday=%d(%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }

    tsp = localtime_r((time_t *)&tv.tv_sec, &ts);
    if (!tsp) {
        (void)fprintf(stderr, "%s[%d]: localtime=%p(%d)\n",
                      __FILE__, __LINE__, tsp, errno);
        return;
    }

    ascret = asctime_r(&ts, d_buf);
    if (!ascret) {
        (void)fprintf(stderr, "%s[%d]: ascret=%p(%d)\n",
                      __FILE__, __LINE__, ascret, errno);
        return;
    }

    retval = sscanf(d_buf, "%s %s %s %s", dammy, mon, day, hour);
    if (retval != 4) {
        (void)fprintf(stderr, "%s[%d]: sscanf=%d(%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }

    retval = gethostname(h_buf, sizeof(h_buf));
    if (retval < 0) {
        (void)fprintf(stderr, "%s[%d]: gethostname=%d(%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }

    tid = pthread_self();
    if (tid)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu",
                       (unsigned long int)tid);

    (void)fprintf(fp, "%s %s%s %s.%ld %s %s[%d]: %s[%d]: ppid=%d%s: %s(",
                  mon, (strlen(day) == 1) ? " " : "", day, hour,
                  tv.tv_usec, h_buf, pname ? : "", getpid(),
                  fname, line, getppid(), tid ? t_buf : "", func);

    va_start(ap, format);
    retval = vfprintf(fp, format, ap);
    va_end(ap);
    if (retval < 0) {
        (void)fprintf(stderr, "%s[%d]: vfprintf=%d(%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }

    (void)fprintf(fp, "): %m(%d)\n", errsv);

    errno = 0; /* errno初期化 */
}

/**
 * 標準エラー出力にHEXダンプ
 *
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len 長さ
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @retval EX_NG エラー
 */
int
dump_log(const void *buf, const size_t len, const char *format, ...)
{
    int retval = 0;                   /* 戻り値 */
    int pt = 0;                       /* アドレス用変数 */
    unsigned char *p;                 /* バッファポインタ */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */

    if (!buf) {
        fprintf(stderr, "%s[%d]: buf=%p(%d)\n",
                __FILE__, __LINE__, buf, errno);
        return EX_NG;
    }
    p = (unsigned char *)buf;

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        fprintf(stderr, "%s[%d]: vsnprintf=%d(%d)\n",
                __FILE__, __LINE__, retval, errno);
        return EX_NG;
    }

    (void)fprintf(stderr, "%s\n", message);
    (void)fprintf(stderr, "%s%s",
                  "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F ",
                  "0123456789ABCDEF\n");
    (void)fprintf(stderr, "%s%s",
                  "--------   ---- ---- ---- ---- ---- ---- ---- ---- ",
                  "----------------\n");

    unsigned int i, j;
    for (i = 0; i < len; ) {
        (void)fprintf(stderr, "%08X : ", pt);
        for (j = 0; j < 16; j++) {
            if ((i + j) >= len)
                (void)fprintf(stderr, "  %s", (j % 2 == 1 ? " " : ""));
            else
                (void)fprintf(stderr, "%02x%s",
                              (unsigned int)*(p + i + j),
                              (j % 2 == 1 ? " " : ""));
        }
        for (j = 0; (i < len) && (j < 16); i++, j++) {
            (void)fprintf(stderr, "%c",
                          *(p + i) < ' ' || '~' < *(p + i) ? '.' : *(p + i));
        }
        (void)fprintf(stderr, "\n");
        pt += j;
    }
    (void)fprintf(stderr, "\n");

    return EX_OK;
}

/**
 * シスログにHEXダンプ
 *
 * @param[in] level ログレベル
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len バッファサイズ
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @retval EX_NG エラー
 */
int dump_sys(const int level,
             const char *pname,
             const char *fname,
             const int line,
             const char *func,
             const void *buf,
             const size_t len,
             const char *format, ...)
{
    int retval = 0;                   /* 戻り値 */
    int pt = 0;                       /* アドレス用変数 */
    unsigned char *p;                 /* バッファポインタ */
    char hexdump[68];                 /* ログ出力用バッファ */
    char tmp[4] = {0};                /* 一時バッファ */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */
    size_t hsize = sizeof(hexdump);   /* hexdump配列サイズ */
    size_t tsize = sizeof(tmp);       /* tmp配列サイズ */

    /* シスログオープン */
    openlog(pname, SYS_OPT, SYS_FACILITY);

    if (!buf) {
        syslog(level, "%s[%d]: buf=%p, format=%p, len=%u",
               __FILE__, __LINE__, buf, format, len);
        return EX_NG;
    }
    p = (unsigned char *)buf;

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(level, "%s[%d]: vsnprintf=%d(%d)",
               __FILE__, __LINE__, retval, errno);
        return EX_NG;
    }

    unsigned int i, j;
    for (i = 0; i < len; ) {
        /* 初期化 */
        (void)memset(hexdump, 0, hsize);
        (void)snprintf(hexdump, hsize, "%08X : ", pt);
        for (j = 0; j < 16; j++) {
            if ((i + j) >= len ) {
                (void)snprintf(tmp, tsize,
                               "  %s", (j % 2 == 1 ? " " : ""));
                (void)strncat(hexdump, tmp,
                              (hsize - strlen(hexdump) - 1));
            } else {
                (void)snprintf(tmp, tsize, "%02x%s",
                               (unsigned int)*(p + i + j),
                               (j % 2 == 1 ? " " : ""));
                (void)strncat(hexdump, tmp,
                              (hsize - strlen(hexdump) - 1));
            }
        }
        for (j = 0; (i < len) && (j < 16); i++, j++) {
            (void)snprintf(tmp, tsize, "%c",
                           (*(p + i) < ' ' || '~' < *(p + i) ?
                            '.' : *(p + i)));
            (void)strncat(hexdump, tmp,
                          (hsize - strlen(hexdump) - 1));
        }
        syslog(level, "%s[%d]: %s(%s): %s",
               fname, line, func, message, hexdump);
        pt += j;
    }

    /* シスログクローズ */
    closelog();

    return EX_OK;
}

/**
 * ファイルにバイナリ出力
 *
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len バッファサイズ
 * @retval EX_NG エラー
 */
int dump_file(const char *pname,
              const char *fname,
              const char *buf,
              const size_t len)
{
    FILE *fp = NULL; /* ファイルディスクリプタ */
    size_t wret = 0; /* fwrite戻り値 */
    int retval = 0;  /* 戻り値 */

    /* シスログオープン */
    openlog(pname, SYS_OPT, SYS_FACILITY);

    if (!buf) {
        syslog(SYS_PRIO, "%s[%d]: buf=%p, len=%u(%d)",
               __FILE__, __LINE__, buf, len, errno);
        return EX_NG;
    }

    fp = fopen(fname, "wb");
    if (!fp) {
        syslog(SYS_PRIO, "%s[%d]: fopen=%p(%d)",
               __FILE__, __LINE__, fp, errno);
        return EX_NG;
    }

    wret = fwrite(buf, len, 1, fp);
    if (wret != 1) {
        syslog(SYS_PRIO, "%s[%d]: fwrite=%p(%d)",
               __FILE__, __LINE__, fp, errno);
        return EX_NG;
    }

    retval = fflush(fp);
    if (retval == EOF) {
        syslog(SYS_PRIO, "%s[%d]: fflush=%p(%d)",
               __FILE__, __LINE__, fp, errno);
    }

    retval = fclose(fp);
    if (retval == EOF) {
        syslog(SYS_PRIO, "%s[%d]: fclose=%p(%d)",
               __FILE__, __LINE__, fp, errno);
        return EX_NG;
    }

    /* シスログクローズ */
    closelog();

    return EX_OK;
}

/**
 * バックトレース出力
 *
 * @return なし
 * @attention backtrace() 関数の size を大きくした場合,
 * クラッシュするかもしれない.
 */
void
print_trace(void)
{
    void *array[10] = {0}; /* 配列 */
    size_t size = 0;       /* サイズ */
    char **strings = NULL; /* 文字列 */

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    (void)fprintf(stderr, "Obtained %zd stack frames.\n", size);

    size_t i;
    for (i = 0; i < size; i++)
        (void)fprintf(stderr, "%p, %s\n", array[i], strings[i]);

    if (strings)
        free(strings);
    strings = NULL;
}


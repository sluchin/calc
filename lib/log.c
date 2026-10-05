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

#include <stdio.h>     /* fprintf vsprintf */
#include <string.h>    /* memset strrchr strerror_r */
#include <stdlib.h>    /* free */
#include <stdarg.h>    /* va_list va_start va_end */
#include <time.h>      /* tm */
#include <sys/time.h>  /* timeval */
#include <unistd.h>    /* getpid gethostname */
#include <sys/types.h> /* getpid */
#include <pthread.h>   /* pthread_self */
#include <errno.h>     /* errno */
#ifdef HAVE_EXECINFO
#  include <execinfo.h> /* backtrace */
#endif

#include "def.h"
#include "term.h"
#include "log.h"

#define MAX_HOST_SIZE 25u  /**< 最大ホスト文字列サイズ */
#define MAX_MES_SIZE  256u /**< 最大メッセージサイズ */
#define STACK_SIZE    100u /**< スタックサイズ */
#define MAX_PROGNAME  25u  /**< 最大プログラム名文字列長 */

/** syslog にメッセージを出力 (ファイル名, 行番号, 関数名, errno 付き) */
#define SYSMSG(lv, fmt, ...) \
    syslog(lv, "%s[%d]: %s: " fmt "(%d)", __FILE__, __LINE__, __func__, ##__VA_ARGS__, errno)

/** 標準エラー出力にメッセージを出力 (ファイル名, 行番号, 関数名, errno 付き) */
#define LOGMSG(fmt, ...)                                                                          \
    (void)fprintf(stderr, "%s[%d]: %s: " fmt "(%d)", __FILE__, __LINE__, __func__, ##__VA_ARGS__, \
                  errno)

/** エラーメッセージ用バッファのサイズ */
#define ERRMSG_SIZE 64u

/**
 * エラー番号のメッセージ取得 (スレッドセーフ)
 *
 * strerror() は, 静的なバッファを使うことがあり, スレッドセーフではない.
 * strerror_r() には, 戻り値が違う 2 つの版がある (GNU 版: char * を返す, XSI 版: int を返す)
 * ので, _GNU_SOURCE が有効な glibc では GNU 版, そのほかでは XSI 版として扱う.
 *
 * @param[in] errnum エラー番号
 * @param[out] buf メッセージを書き込むバッファ
 * @param[in] size バッファのサイズ
 * @return メッセージ (buf と同じとは限らない)
 */
static const char *
get_errmsg(const int errnum, char *buf, const size_t size)
{
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
    return strerror_r(errnum, buf, size);
#else
    int retval = 0; /* 戻り値 */

    retval = strerror_r(errnum, buf, size);
    if (retval != 0)
        (void)snprintf(buf, size, "Unknown error %d", errnum);
    return buf;
#endif
}

/** 月省略名配列 */
static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* 内部変数 */
static char progname[MAX_PROGNAME] = {0}; /**< プログラム名 */

/* 内部関数 */

/**
 * @brief プログラム名設定
 *
 * @param[in] name プログラム名
 */
void
set_progname(const char *name)
{
    const char *ptr = NULL; /* strrchr戻り値 */

    if (progname[0] == '\0') { /* 一度のみ設定される */
        ptr = strrchr(name, '/');
        if (ptr != NULL)
            (void)strncpy(progname, ptr + 1, sizeof(progname) - 1u);
        else
            (void)strncpy(progname, name, sizeof(progname) - 1u);
    }
}

/**
 * @brief プログラム名取得
 *
 * @return プログラム名
 */
char *
get_progname(void)
{
    return progname;
}

/**
 * @brief シスログ出力
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 */
void
system_log(const int level,
           const int option,
           const char *pname,
           const char *fname,
           const int line,
           const char *func,
           const char *format,
           ...)
{
    int errsv = errno;                /* errno退避 */
    char ebuf[ERRMSG_SIZE] = {0};     /* エラーメッセージ */
    int retval = 0;                   /* 戻り値 */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */
    pthread_t tid = 0;                /* スレッドID */
    /* スレッドID用バッファ
     * 64bit ULONG_MAX: 18446744073709551615UL
     * 32bit ULONG_MAX: 4294967295UL */
    char t_buf[sizeof(", tid=18446744073709551615")] = {0};

    /* シスログオープン */
    openlog(pname, option, SYS_FACILITY);

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        SYSMSG(level, "vsnprintf");
        return;
    }

    tid = pthread_self();
    if (tid != 0)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu", (unsigned long)tid);

    syslog(level, "ppid=%d%s: %s[%d]: %s(%s): %s(%d)", getppid(), ((tid != 0) ? t_buf : ""), fname,
           line, func, message, get_errmsg(errsv, ebuf, sizeof(ebuf)), errsv);

    /* シスログクローズ */
    closelog();
    errno = 0; /* errno初期化 */
}

/**
 * @brief シスログ出力(デバッグ用)
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 */
void
system_dbg_log(const int level,
               const int option,
               const char *pname,
               const char *fname,
               const int line,
               const char *func,
               const char *format,
               ...)
{
    int errsv = errno;                /* errno退避 */
    char ebuf[ERRMSG_SIZE] = {0};     /* エラーメッセージ */
    int retval = 0;                   /* 戻り値 */
    struct tm t;                      /* tm構造体 */
    struct tm *tp = NULL;             /* localtime_r戻り値 */
    struct timeval tv;                /* timeval構造体 */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */
    pthread_t tid = 0;                /* スレッドID */
    /* スレッドID用バッファ
     * 64bit ULONG_MAX: 18446744073709551615UL
     * 32bit ULONG_MAX: 4294967295UL */
    char t_buf[sizeof(", tid=18446744073709551615")] = {0};

    timerclear(&tv);
    (void)memset(&t, 0, sizeof(struct tm));

    /* シスログオープン */
    openlog(pname, option, SYS_FACILITY);

    retval = gettimeofday(&tv, NULL);
    if (retval < 0) {
        SYSMSG(level, "gettimeofday");
        return;
    }

    tp = localtime_r(&tv.tv_sec, &t);
    if (tp == NULL) {
        SYSMSG(level, "localtime_r");
        return;
    }

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        SYSMSG(level, "vsnprintf");
        return;
    }

    tid = pthread_self();
    if (tid != 0)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu", (unsigned long)tid);

    syslog(level, "ppid=%d%s: %02d.%06ld: %s[%d]: %s(%s): %s(%d)", getppid(),
           ((tid != 0) ? t_buf : ""), t.tm_sec, tv.tv_usec, fname, line, func, message,
           get_errmsg(errsv, ebuf, sizeof(ebuf)), errsv);

    /* シスログクローズ */
    closelog();
    errno = 0; /* errno初期化 */
}

/**
 * @brief 標準エラー出力にログ出力
 *
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 */
void
stderr_log(
    const char *pname, const char *fname, const int line, const char *func, const char *format, ...)
{
    int errsv = errno;               /* errno退避 */
    char ebuf[ERRMSG_SIZE] = {0};    /* エラーメッセージ */
    FILE *fp = stderr;               /* 標準エラー出力 */
    int retval = 0;                  /* 戻り値 */
    struct tm t;                     /* tm構造体 */
    struct tm *tp = NULL;            /* localtime_r戻り値 */
    struct timeval tv;               /* timeval構造体 */
    va_list ap;                      /* va_list */
    char h_buf[MAX_HOST_SIZE] = {0}; /* ホスト */
    pthread_t tid = 0;               /* スレッドID */
    /* スレッドID用バッファ
     * 64bit ULONG_MAX: 18446744073709551615UL
     * 32bit ULONG_MAX: 4294967295UL */
    char t_buf[sizeof(", tid=18446744073709551615")] = {0};

    timerclear(&tv);
    (void)memset(&t, 0, sizeof(struct tm));

    retval = gettimeofday(&tv, NULL);
    if (retval < 0) {
        LOGMSG("gettimeofday");
        return;
    }

    tp = localtime_r((time_t *)&tv.tv_sec, &t);
    if (tp == NULL) {
        LOGMSG("localtime_r");
        return;
    }

    retval = gethostname(h_buf, sizeof(h_buf));
    if (retval < 0) {
        LOGMSG("gethostname");
        return;
    }

    tid = pthread_self();
    if (tid != 0)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu", (unsigned long)tid);

    (void)fprintf(fp,
                  "%s %02d %02d:%02d:%02d.%06ld "
                  "%s %s[%d]: %s[%d]: ppid=%d%s: %s(",
                  (((0 <= t.tm_mon) && (t.tm_mon < (int)NELEMS(mon))) ? mon[t.tm_mon] : ""),
                  t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec, h_buf,
                  ((pname != NULL) ? pname : ""), getpid(), fname, line, getppid(),
                  ((tid != 0) ? t_buf : ""), func);

    va_start(ap, format);
    retval = vfprintf(fp, format, ap);
    va_end(ap);
    if (retval < 0) {
        LOGMSG("vfprintf");
        return;
    }

    (void)fprintf(fp, "): %s(%d)\n", get_errmsg(errsv, ebuf, sizeof(ebuf)), errsv);

    errno = 0; /* errno初期化 */
}

/**
 * @brief 標準エラー出力にHEXダンプ
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
    FILE *fp = stderr;                /* 標準エラー出力 */
    int retval = 0;                   /* 戻り値 */
    unsigned int pt = 0u;             /* アドレス用変数 */
    const unsigned char *p = NULL;    /* バッファポインタ */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */

    if (buf == NULL)
        return EX_NG;

    p = (const unsigned char *)buf;

    /* メッセージ作成 */
    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        LOGMSG("vsnprintf");
        return EX_NG;
    }

    /* メッセージと, ダンプのヘッダを出力 */
    (void)fprintf(fp, "%s\n", message);
    (void)fprintf(fp, "%s%s", "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F ",
                  "0123456789ABCDEF\n");
    (void)fprintf(fp, "%s%s", "--------   ---- ---- ---- ---- ---- ---- ---- ---- ",
                  "----------------\n");

    /* 16 バイトごとに, アドレス, 16 進数 (2 バイトごとに空白), 文字を出力 */
    unsigned int i, j;
    for (i = 0u; i < len;) {
        (void)fprintf(fp, "%08X : ", pt);
        for (j = 0u; j < 16u; j++) {
            if ((i + j) >= len) /* 16 バイトに満たない分は, 空白で埋める */
                (void)fprintf(fp, "  %s", (((j % 2u) == 1u) ? " " : ""));
            else
                (void)fprintf(fp, "%02x%s", (unsigned int)*(p + i + j),
                              (((j % 2u) == 1u) ? " " : ""));
        }
        for (j = 0u; (i < len) && (j < 16u); i++, j++) {
            (void)fprintf(fp, "%c", (((*(p + i) < ' ') || ('~' < *(p + i))) ? '.' : *(p + i)));
        }
        (void)fprintf(fp, "\n");
        pt += j;
    }
    (void)fprintf(fp, "\n");

    return EX_OK;
}

/**
 * @brief シスログにHEXダンプ
 *
 * @param[in] level ログレベル
 * @param[in] option オプショ ン
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
int
dump_sys(const int level,
         const int option,
         const char *pname,
         const char *fname,
         const int line,
         const char *func,
         const void *buf,
         const size_t len,
         const char *format,
         ...)
{
    int retval = 0;                   /* 戻り値 */
    unsigned int pt = 0u;             /* アドレス用変数 */
    const unsigned char *p = NULL;    /* バッファポインタ */
    char hexdump[68];                 /* ログ出力用バッファ */
    char tmp[4] = {0};                /* 一時バッファ */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */
    size_t hsize = sizeof(hexdump);   /* hexdump配列サイズ */
    size_t tsize = sizeof(tmp);       /* tmp配列サイズ */

    /* シスログオープン */
    openlog(pname, option, SYS_FACILITY);

    if (buf == NULL)
        return EX_NG;

    p = (const unsigned char *)buf;

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        SYSMSG(level, "vsnprintf");
        return EX_NG;
    }

    unsigned int i, j;
    for (i = 0u; i < len;) {
        /* 初期化 */
        (void)memset(hexdump, 0, hsize);
        (void)snprintf(hexdump, hsize, "%08X : ", pt);
        /* ダンプの表示 */
        for (j = 0u; j < 16u; j++) {
            (void)memset(tmp, 0, tsize);
            if ((i + j) >= len) {
                (void)snprintf(tmp, tsize, "  %s", (((j % 2u) == 1u) ? " " : ""));
                (void)strncat(hexdump, tmp, (hsize - strlen(hexdump) - 1u));
            } else {
                (void)snprintf(tmp, tsize, "%02x%s", (unsigned int)*(p + i + j),
                               (((j % 2u) == 1u) ? " " : ""));
                (void)strncat(hexdump, tmp, (hsize - strlen(hexdump) - 1u));
            }
        }
        /* アスキー文字の表示 */
        for (j = 0u; (i < len) && (j < 16u); i++, j++) {
            (void)memset(tmp, 0, tsize);
            (void)snprintf(tmp, tsize, "%c",
                           (((*(p + i) < ' ') || ('~' < *(p + i))) ? '.' : *(p + i)));
            (void)strncat(hexdump, tmp, (hsize - strlen(hexdump) - 1u));
        }
        syslog(level, "%s[%d]: %s(%s): %s", fname, line, func, message, hexdump);
        pt += j;
    }

    /* シスログクローズ */
    closelog();

    return EX_OK;
}

/**
 * @brief ファイルにバイナリ出力
 *
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len バッファサイズ
 * @retval EX_NG エラー
 */
int
dump_file(const char *pname, const char *fname, const char *buf, const size_t len)
{
    FILE *fp = NULL;  /* ファイルディスクリプタ */
    size_t wret = 0u; /* fwrite戻り値 */
    int retval = 0;   /* 戻り値 */

    /* シスログオープン */
    openlog(pname, LOG_PID, SYS_FACILITY);

    if (buf == NULL)
        return EX_NG;

    fp = fopen(fname, "wb");
    if (fp == NULL) {
        SYSMSG(LOG_INFO, "fopen");
        return EX_NG;
    }

    wret = fwrite(buf, len, 1u, fp);
    if (wret != 1u) {
        SYSMSG(LOG_INFO, "fwrite");
        (void)fclose(fp); /* ファイルを閉じないと, ファイルポインタがリークする */
        return EX_NG;
    }

    retval = fflush(fp);
    if (retval == EOF) {
        SYSMSG(LOG_INFO, "fflush");
    }

    retval = fclose(fp);
    if (retval == EOF) {
        SYSMSG(LOG_INFO, "fclose");
        return EX_NG;
    }

    /* シスログクローズ */
    closelog();

    return EX_OK;
}

/**
 * @brief バックトレースシスログ出力
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 */
#ifdef HAVE_EXECINFO
void
systrace(const int level,
         const int option,
         const char *pname,
         const char *fname,
         const int line,
         const char *func)
{
    int errsv = errno;              /* エラー番号退避 */
    char ebuf[ERRMSG_SIZE] = {0};   /* エラーメッセージ */
    void *buffer[STACK_SIZE] = {0}; /* 配列 */
    int size = 0;                   /* サイズ */
    char **strings = NULL;          /* 文字列 */

    /* シスログオープン */
    openlog(pname, option, SYS_FACILITY);

    size = backtrace(buffer, STACK_SIZE);
    strings = backtrace_symbols(buffer, size);
    if (strings == NULL) {
        SYSMSG(level, "backtrace_symbols: size=%d", size);
        return;
    }

    syslog(level, "%s[%d]: %s: Obtained %d stack frames: %s(%d)", fname, line, func, size,
           get_errmsg(errsv, ebuf, sizeof(ebuf)), errsv);

    int i;
    for (i = 0; i < size; i++) {
        syslog(level, "%s[%d]: %s: %s: %s(%d)", fname, line, func, strings[i],
               get_errmsg(errsv, ebuf, sizeof(ebuf)), errsv);
    }

    /* シスログクローズ */
    closelog();

    /* memfree() は, デバッグ版で systrace() を呼ぶので, 再帰しないよう, free() を使う */
    if (strings != NULL)
        free(strings);
    strings = NULL;
}
#endif

/**
 * @brief バックトレース出力
 */
#ifdef HAVE_EXECINFO
void
print_trace(void)
{
    void *buffer[STACK_SIZE] = {0}; /* 配列 */
    int size = 0;                   /* サイズ */
    char **strings = NULL;          /* 文字列 */

    /* 呼び出し履歴を取得して, 標準エラー出力に出力する */
    size = backtrace(buffer, STACK_SIZE);
    strings = backtrace_symbols(buffer, size);

    (void)fprintf(stderr, "Obtained %d stack frames.\n", size);

    int i;
    for (i = 0; i < size; i++)
        (void)fprintf(stderr, "%s\n", strings[i]);

    /* memfree() は, デバッグ版で systrace() を呼ぶので, 再帰しないよう, free() を使う */
    if (strings != NULL)
        free(strings);
    strings = NULL;
}
#endif

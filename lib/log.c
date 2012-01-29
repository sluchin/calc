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
#include <string.h>    /* memset strrchr */
#include <stdlib.h>    /* free */
#include <stdarg.h>    /* va_list va_start va_end */
#include <time.h>      /* tm */
#include <sys/time.h>  /* timeval */
#include <unistd.h>    /* getpid gethostname */
#include <sys/types.h> /* getpid */
#include <pthread.h>   /* pthread_self */
#ifdef HAVE_EXECINFO
#include <execinfo.h>  /* backtrace */
#endif
#include <errno.h>     /* errno */

#include "def.h"
#include "term.h"
#include "log.h"

#define MAX_HOST_SIZE  25 /**< 最大ホスト文字列サイズ */
#define MAX_MES_SIZE  256 /**< 最大メッセージサイズ */
#define STACK_SIZE    100 /**< スタックサイズ */
#define MAX_PROGNAME   25 /**< 最大プログラム名文字列長 */

#define SYSMSG(lv, fmt, ...)                                    \
    syslog(lv, "%s[%d]: %s: " fmt "(%d)",                       \
           __FILE__, __LINE__, __func__, ## __VA_ARGS__, errno)

#define LOGMSG(fmt, ...)                                                \
    (void)fprintf(stderr, "%s[%d]: %s: " fmt "(%d)",                    \
                  __FILE__, __LINE__, __func__, ## __VA_ARGS__, errno)

/** 月省略名配列 */
static const char *mon[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* 内部変数 */
static char progname[MAX_PROGNAME] = {0}; /**< プログラム名 */

/* 内部関数 */

/**
 * プログラム名設定
 *
 * @param[in] name プログラム名
 * @return なし
 */
void
set_progname(const char *name)
{
    char *ptr = NULL; /* strrchr戻り値 */

    if (progname[0] == '\0') { /* 一度のみ設定される */
        /* g++ではchar*にキャストしなければ警告される */
        ptr = strrchr((char *)name, '/');
        if (ptr)
            (void)strncpy(progname, ptr + 1, sizeof(progname) - 1);
        else
            (void)strncpy(progname, name, sizeof(progname) - 1);
    }
}

/**
 * プログラム名取得
 *
 * @return プログラム名
 */
char *
get_progname(void)
{
    return progname;
}

/**
 * シスログ出力
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void
system_log(const int level,
           const int option,
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
    openlog(pname, option, SYS_FACILITY);

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        SYSMSG(level, "vsnprintf");
        return;
    }

    tid = pthread_self();
    if (tid)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu", (ulong)tid);

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
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void
system_dbg_log(const int level,
               const int option,
               const char *pname,
               const char *fname,
               const int line,
               const char *func,
               const char *format, ...)
{
    int errsv = errno;                /* errno退避 */
    int retval = 0;                   /* 戻り値 */
    struct tm t;                      /* tm構造体 */
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

    if (gettimeofday(&tv, NULL) < 0) {
        SYSMSG(level, "gettimeofday");
        return;
    }

    if (localtime_r(&tv.tv_sec, &t) == NULL) {
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
    if (tid)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu", (ulong)tid);

    syslog(level, "ppid=%d%s: %02d.%06ld: %s[%d]: %s(%s): %m(%d)",
           getppid(), tid ? t_buf : "",
           t.tm_sec, tv.tv_usec, fname, line, func, message, errsv);

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
void
stderr_log(const char *pname,
           const char *fname,
           const int line,
           const char *func,
           const char *format, ...)
{
    int errsv = errno;               /* errno退避 */
    FILE *fp = stderr;               /* 標準エラー出力 */
    int retval = 0;                  /* 戻り値 */
    struct tm t;                     /* tm構造体 */
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

    if (gettimeofday(&tv, NULL) < 0) {
        LOGMSG("gettimeofday");
        return;
    }

    if (localtime_r((time_t *)&tv.tv_sec, &t) == NULL) {
        LOGMSG("localtime_r");
        return;
    }

    if (gethostname(h_buf, sizeof(h_buf)) < 0) {
        LOGMSG("gethostname");
        return;
    }

    tid = pthread_self();
    if (tid)
        (void)snprintf(t_buf, sizeof(t_buf), ", tid=%lu", (ulong)tid);

    (void)fprintf(fp,
                  "%s %02d %02d:%02d:%02d.%06ld " \
                  "%s %s[%d]: %s[%d]: ppid=%d%s: %s(",
                  (0 <= t.tm_mon && t.tm_mon < (int)NELEMS(mon)) ?
                  mon[t.tm_mon] : "",
                  t.tm_mday, t.tm_hour, t.tm_min,
                  t.tm_sec, tv.tv_usec, h_buf, pname ? : "", getpid(),
                  fname, line, getppid(), tid ? t_buf : "", func);

    va_start(ap, format);
    retval = vfprintf(fp, format, ap);
    va_end(ap);
    if (retval < 0) {
        LOGMSG("vfprintf");
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
    FILE *fp = stderr;                /* 標準エラー出力 */
    int retval = 0;                   /* 戻り値 */
    int pt = 0;                       /* アドレス用変数 */
    unsigned char *p = NULL;          /* バッファポインタ */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */

    if (!buf)
        return EX_NG;

    p = (unsigned char *)buf;

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        LOGMSG("vsnprintf");
        return EX_NG;
    }

    (void)fprintf(fp, "%s\n", message);
    (void)fprintf(fp, "%s%s",
                  "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F ",
                  "0123456789ABCDEF\n");
    (void)fprintf(fp, "%s%s",
                  "--------   ---- ---- ---- ---- ---- ---- ---- ---- ",
                  "----------------\n");

    unsigned int i, j;
    for (i = 0; i < len; ) {
        (void)fprintf(fp, "%08X : ", pt);
        for (j = 0; j < 16; j++) {
            if ((i + j) >= len)
                (void)fprintf(fp, "  %s", (j % 2 == 1 ? " " : ""));
            else
                (void)fprintf(fp, "%02x%s",
                              (unsigned int)*(p + i + j),
                              (j % 2 == 1 ? " " : ""));
        }
        for (j = 0; (i < len) && (j < 16); i++, j++) {
            (void)fprintf(fp, "%c",
                          *(p + i) < ' ' || '~' < *(p + i) ? '.' : *(p + i));
        }
        (void)fprintf(fp, "\n");
        pt += j;
    }
    (void)fprintf(fp, "\n");

    return EX_OK;
}

/**
 * シスログにHEXダンプ
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
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
         const char *format, ...)
{
    int retval = 0;                   /* 戻り値 */
    int pt = 0;                       /* アドレス用変数 */
    unsigned char *p = NULL;          /* バッファポインタ */
    char hexdump[68];                 /* ログ出力用バッファ */
    char tmp[4] = {0};                /* 一時バッファ */
    char message[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                       /* va_list */
    size_t hsize = sizeof(hexdump);   /* hexdump配列サイズ */
    size_t tsize = sizeof(tmp);       /* tmp配列サイズ */

    /* シスログオープン */
    openlog(pname, option, SYS_FACILITY);

    if (!buf)
        return EX_NG;

    p = (unsigned char *)buf;

    va_start(ap, format);
    retval = vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    if (retval < 0) {
        SYSMSG(level, "vsnprintf");
        return EX_NG;
    }

    unsigned int i, j;
    for (i = 0; i < len; ) {
        /* 初期化 */
        (void)memset(hexdump, 0, hsize);
        (void)snprintf(hexdump, hsize, "%08X : ", pt);
        /* ダンプの表示 */
        for (j = 0; j < 16; j++) {
            (void)memset(tmp, 0, tsize);
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
        /* アスキー文字の表示 */
        for (j = 0; (i < len) && (j < 16); i++, j++) {
            (void)memset(tmp, 0, tsize);
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
int
dump_file(const char *pname,
          const char *fname,
          const char *buf,
          const size_t len)
{
    FILE *fp = NULL; /* ファイルディスクリプタ */
    size_t wret = 0; /* fwrite戻り値 */
    int retval = 0;  /* 戻り値 */

    /* シスログオープン */
    openlog(pname, LOG_PID, SYS_FACILITY);

    if (!buf)
        return EX_NG;

    fp = fopen(fname, "wb");
    if (!fp) {
        SYSMSG(LOG_INFO, "fopen");
        return EX_NG;
    }

    wret = fwrite(buf, len, 1, fp);
    if (wret != 1) {
        SYSMSG(LOG_INFO, "fwrite");
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

#ifdef HAVE_EXECINFO
/**
 * バックトレースシスログ出力
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @return なし
 */
void
systrace(const int level,
         const int option,
         const char *pname,
         const char *fname,
         const int line,
         const char *func)
{
    int errsv = errno;              /* エラー番号退避 */
    void *buffer[STACK_SIZE] = {0}; /* 配列 */
    size_t size = 0;                /* サイズ */
    char **strings = NULL;          /* 文字列 */

    /* シスログオープン */
    openlog(pname, option, SYS_FACILITY);

    size = backtrace(buffer, STACK_SIZE);
    strings = backtrace_symbols(buffer, size);
    if (!strings) {
        SYSMSG(level, "backtrace_symbols: size=%zu", size);
        return;
    }

    syslog(level, "%s[%d]: %s: Obtained %zd stack frames: %m(%d)",
           fname, line, func, size, errsv);

    size_t i;
    for (i = 0; i < size; i++) {
        syslog(level, "%s[%d]: %s: %s: %m(%d)",
               fname, line, func, strings[i], errsv);
    }

    /* シスログクローズ */
    closelog();

    if (strings)
        free(strings);
    strings = NULL;
}

/**
 * バックトレース出力
 *
 * @return なし
 */
void
print_trace(void)
{
    void *buffer[STACK_SIZE] = {0}; /* 配列 */
    size_t size = 0;                /* サイズ */
    char **strings = NULL;          /* 文字列 */

    size = backtrace(buffer, STACK_SIZE);
    strings = backtrace_symbols(buffer, size);

    (void)fprintf(stderr, "Obtained %zd stack frames.\n", size);

    size_t i;
    for (i = 0; i < size; i++)
        (void)fprintf(stderr, "%s\n", strings[i]);

    if (strings)
        free(strings);
    strings = NULL;
}
#endif


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
#include <stdlib.h>    /* exit atexit */
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
#include "term.h"
#include "log.h"

#define MAX_HOST_SIZE  25 /**< 最大ホスト文字列サイズ */
#define MAX_MES_SIZE  256 /**< 最大メッセージサイズ */
#define STACK_SIZE    100 /**< スタックサイズ */

#define SYSMSG(lv, fmt, ...)                                    \
    syslog(lv, "%s[%d]: %s: " fmt "(%d)",                       \
           __FILE__, __LINE__, __func__, ## __VA_ARGS__, errno)

#define LOGMSG(fmt, ...)                                                \
    (void)fprintf(stderr, "%s[%d]: %s: " fmt "(%d)",                    \
                  __FILE__, __LINE__, __func__, ## __VA_ARGS__, errno)

/* 内部変数 */
static char *progname = NULL; /**< プログラム名 */

/* 内部関数 */
/** 月省略名 */
static char *strmon(int mon);
/** プログラム名解放 */
static void destroy_progname(void);

/**
 * プログラム名設定
 *
 * @param[in] name プログラム名
 * @return なし
 */
void
set_progname(char *name)
{
    char *ptr = NULL; /* strrchr戻り値 */

    if (!progname) { /* 一度のみ設定される */
        ptr = strrchr(name, '/');
        if (ptr)
            progname = strdup(ptr + 1);
        else
            progname = strdup(name);

        if (atexit(destroy_progname)) {
            outlog("atexit");
            exit(EXIT_FAILURE);
        }
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
    char *mon = NULL;                /* 月名 */
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

    mon = strmon(t.tm_mon);
    (void)fprintf(fp,
                  "%s %02d %02d:%02d:%02d.%06ld " \
                  "%s %s[%d]: %s[%d]: ppid=%d%s: %s(",
                  mon ? : "", t.tm_mday, t.tm_hour, t.tm_min,
                  t.tm_sec, tv.tv_usec, h_buf, pname ? : "", getpid(),
                  fname, line, getppid(), tid ? t_buf : "", func);

    if (mon)
        free(mon);
    mon = NULL;

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
        SYSMSG(level, "backtrace_symbols: size=%d", size);
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

/**
 * ターミナル属性シスログ出力
 *
 * @param[in] level ログレベル
 * @param[in] option オプション
 * @param[in] pname プログラム名
 * @param[in] fname ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] fd ファイルディスクリプタ
 * @return なし
 */
void
sys_print_termattr(const int level, const int option,
                   const char *pname, const char *fname,
                   const int line, const char *func, int fd)
{
    struct termios mode;
    char *result = NULL;

    (void)memset(&mode, 0, sizeof(struct termios));

    result = get_termattr(STDIN_FILENO, &mode);
    if (!result)
        return;

    openlog(pname, option, SYS_FACILITY);
    syslog(level, "%s[%d]: %s: %s", fname, line, func, result);

    closelog();

    if (result)
        free(result);
    result = NULL;
}

/**
 * 月省略名
 *
 * @param[in] mon 月
 * @return 月省略名
 * @attention 戻り値ポインタは解放しなければならない
 */
static char *
strmon(int mon)
{
    switch(mon) {
    case  0:
        return strdup("Jan");
    case  1:
        return strdup("Feb");
    case  2:
        return strdup("Mar");
    case  3:
        return strdup("Apr");
    case  4:
        return strdup("May");
    case  5:
        return strdup("Jun");
    case  6:
        return strdup("Jul");
    case  7:
        return strdup("Aug");
    case  8:
        return strdup("Sep");
    case  9:
        return strdup("Oct");
    case 10:
        return strdup("Nov");
    case 11:
        return strdup("Dec");
    default:
        return NULL;
    }
    return NULL;
}

/**
 * プログラム名解放
 *
 * @return なし
 */
static void
destroy_progname(void)
{
    if (progname)
        free(progname);
    progname = NULL;
}

#ifdef UNITTEST
void
test_init_log(testlog *log)
{
    log->strmon = strmon;
    log->destroy_progname = destroy_progname;
    log->progname = progname;
}
#endif


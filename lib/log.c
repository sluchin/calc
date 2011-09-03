/**
 * @file log.c
 * @brief ログ出力
 *
 * @see log.h
 * @author higashi
 * @date 2010-06-22 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010 Tetsuya Higashi. All Rights Reserved.
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

#include <stdio.h>      /* fprintf vsprintf */
#include <string.h>     /* memset */
#include <stdlib.h>     /* exit */
#include <stdarg.h>     /* va_list va_start va_end */
#include <errno.h>      /* errno */
#include <time.h>       /* struct tm */
#include <sys/time.h>   /* struct timeval */
#include <sys/types.h>  /* getpid */
#include <unistd.h>     /* getpid gethostname */
#include <ctype.h>      /* isprint */
#include <pthread.h>    /* pthread_self */
#include <syslog.h>     /* syslog */

#include "log.h"

#define MAX_HOST_SIZE   25 /**< 最大ホスト文字列サイズ */
#define MAX_MES_SIZE   256 /**< 最大メッセージサイズ */

/**
 * シスログ出力
 *
 * @param[in] prog_name プログラム名
 * @param[in] filename ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void system_log(const char *prog_name,
                const char *filename,
                const unsigned long line,
                const char *func,
                const char *format, ...)
{
    int err_no = errno;             /* errno退避 */
    int retval = 0;                 /* 戻り値 */
    char m_buf[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                     /* va_list */

    if (!format) {
        outlog("format[%p]", format);
        return;
    }

    /* シスログオープン */
    openlog (prog_name, LOG_PID, LOG_SYSLOG);

    va_start(ap, format);
    retval = vsnprintf(m_buf, sizeof(m_buf), format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(SYS_PRIO, "%s[%d]: vsnprintf[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    syslog(SYS_PRIO, "%s[%lu]: %s[%d.%d.%u]: %m(%d): %s",
           filename, line, func, getpid(), getppid(),
           (unsigned int)pthread_self(), err_no, m_buf);

    /* シスログクローズ */
    closelog();
    errno = 0; /* errno初期化 */

}

/**
 * シスログ出力(デバッグ用)
 *
 * @param[in] prog_name プログラム名
 * @param[in] filename ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void system_dbg_log(const char *prog_name,
                    const char *filename,
                    const unsigned long line,
                    const char *func,
                    const char *format, ...)
{
    int err_no = errno;             /* errno退避 */
    int retval = 0;                 /* 戻り値 */
    struct tm ts;                   /* tm構造体 */
    struct tm *tsp;                 /* localtime_r戻り値 */ 
    struct timeval tv;              /* timeval構造体 */
    char m_buf[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    char d_buf[sizeof("00")] = {0}; /* 秒格納用バッファ */
    va_list ap;                     /* va_list */

    if (!format) {
        outlog("format[%p]", format);
        return;
    }
    /* シスログオープン */
    openlog(prog_name, LOG_PID, LOG_SYSLOG);

    retval = gettimeofday (&tv, NULL);
    if (retval < 0) {
        syslog(SYS_PRIO, "%s[%d]: gettimeofday[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    tsp = localtime_r(&tv.tv_sec, &ts);
    if (!tsp) {
        syslog(SYS_PRIO, "%s[%d]: localtime[%p](%d)",
               __FILE__, __LINE__, tsp, errno);
        return;
    }

    retval = strftime(d_buf, sizeof(d_buf), "%S", &ts);
    if (!retval) {
        syslog(SYS_PRIO, "%s[%d] strftime[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    va_start(ap, format);
    retval = vsnprintf(m_buf, sizeof(m_buf), format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(SYS_PRIO, "%s[%d]: vsnprintf[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }
    syslog(SYS_PRIO, "%s[%lu]: %s[%d.%d.%u]: %m[%s.%ld](%d): %s",
           filename, line, func, getpid(), getppid(),
           (unsigned int)pthread_self(), d_buf, tv.tv_usec,
           err_no, m_buf);

    /* シスログクローズ */
    closelog();
    errno = 0;  /* errno初期化 */

}

/**
 * 標準エラー出力にログ出力
 *
 * @param[in] prog_name プログラム名
 * @param[in] filename ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] format フォーマット
 * @param[in] ... 可変引数
 * @return なし
 */
void stderr_log(const char *prog_name,
                const char *filename,
                const unsigned long line,
                const char *func,
                const char *format, ...)
{
    FILE *fp = stderr;  /* 標準エラー出力 */
    int err_no = errno; /* errno退避 */
    int retval = 0;     /* 戻り値 */
    struct tm ts;       /* tm構造体 */
    struct tm *tsp;     /* localtime_rの戻り値 */
    struct timeval tv;  /* timeval構造体 */
    va_list ap;         /* va_list */
    char d_buf[sizeof("xxx 00 00:00:00")] = {0}; /* 時間用バッファ */
    char h_buf[MAX_HOST_SIZE] = {0};             /* ホスト用バッファ */

    if (!format) {
        outlog("format[%p]", format);
        return;
    }

    retval = gettimeofday(&tv, NULL);
    if (retval < 0) {
        (void)fprintf(stderr, "%s[%d] gettimeofday[%d](%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }

    tsp = localtime_r(&tv.tv_sec, &ts);
    if (!tsp) {
        (void)fprintf(stderr, "%s[%d] localtime[%p](%d)\n",
                      __FILE__, __LINE__, tsp, errno);
        return;
    }

    retval = strftime(d_buf, sizeof(d_buf), "%b %d %H:%M:%S", &ts);
    if (!retval) {
        (void)fprintf(stderr, "%s[%d] strftime[%d](%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }

    retval = gethostname(h_buf, sizeof(h_buf));
    if (retval < 0) {
        (void)fprintf(stderr, "%s[%d] gethostname[%d](%d)\n",
                      __FILE__, __LINE__, retval, errno);
        return;
    }
    (void)fprintf(fp, "%s.%ld %s %s %s[%lu]: %s[%d.%d.%u](%d): ",
                  d_buf, tv.tv_usec, h_buf, prog_name,
                  filename, line, func, getpid(), getppid(),
                  (unsigned int)pthread_self(), err_no);

    va_start(ap, format);
    retval = vfprintf(fp, format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(SYS_PRIO, "%s[%d]: vfprintf[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }

    (void)fprintf(fp, "\n");

    errno = 0; /* errno初期化 */
}

/**
 * 標準エラー出力にHEXダンプ
 *
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len 長さ
 * @param[in] format フォーマット
 * @return なし
 */
void
dump_log(const void *buf, const size_t len, const char *format, ...)
{
    int retval = 0;                 /* 戻り値 */
    unsigned int i, k;              /* 汎用変数 */
    int pt = 0;                     /* アドレス用変数 */
    unsigned char *p;               /* バッファポインタ */
    char m_buf[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                     /* va_list */

    if (!buf) {
        outlog("buf[%p] len[%u]", buf, len);
        return;
    }
    p = (unsigned char *)buf;

    va_start(ap, format);
    retval = vsnprintf(m_buf, sizeof(m_buf), format, ap);
    va_end(ap);
    if (retval < 0) {
        fprintf(stderr, "%s[%d]: vsnprintf[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }
    (void)fprintf(stderr, "%s\n", m_buf);


#if 1
    (void)fprintf(stderr,
                  "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F 0123456789ABCDEF\n");
    (void)fprintf(stderr,
                  "--------   ---- ---- ---- ---- ---- ---- ---- ---- ----------------\n");
#endif
    for (i = 0 ; i < len ; )
    {
        (void)fprintf(stderr, "%08X : ", pt);
        for (k = 0; k < 16; k++) {
            if ((i + k) >= len)
                (void)fprintf(stderr, "  %s", (k % 2 == 1 ? " " : ""));
            else
                (void)fprintf(stderr, "%02x%s",
                              (unsigned int)*(p + i + k),
                              (k % 2 == 1 ? " " : ""));
        }
        for (k = 0; (i < len) && (k < 16) ;k++, i++) {
            (void)fprintf(stderr, "%c",
                          (*(p + i) < ' ' || *(p + i) >= 0x80 ?
                           '.' : *(p + i)));
        }
        (void)fprintf(stderr, "\n");
        pt += k;
    }
    (void)fprintf(stderr, "\n");
}

/**
 * シスログにHEXダンプ
 *
 * @param[in] prog_name プログラム名
 * @param[in] filename ファイル名
 * @param[in] line 行番号
 * @param[in] func 関数名
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len 長さ
 * @param[in] format フォーマット
 * @return なし
 */
void dump_sys(const char *prog_name,
              const char *filename,
              const unsigned long line,
              const char *func,
              const void *buf,
              const size_t len,
              const char *format, ...)
{
    int retval = 0;                 /* 戻り値 */
    unsigned int i, k;              /* 汎用変数 */
    static int pt = 0;              /* アドレス用変数 */
    unsigned char *p;               /* バッファポインタ */
    char tmp[4] = {0};              /* 一時バッファ */
    char logmsg[68];                /* ログ出力用バッファ */
    char m_buf[MAX_MES_SIZE] = {0}; /* メッセージ用バッファ */
    va_list ap;                     /* va_list */

    if (!buf || !format) {
        outlog("buf[%p] format[%p] len[%u]", buf, format, len);
        return;
    }
    p = (unsigned char *)buf;

    /* シスログオープン */
    openlog(prog_name, LOG_CONS | LOG_PID, LOG_SYSLOG);

    va_start(ap, format);
    retval = vsnprintf(m_buf, sizeof(m_buf), format, ap);
    va_end(ap);
    if (retval < 0) {
        syslog(SYS_PRIO, "%s[%d]: vsnprintf[%d](%d)",
               __FILE__, __LINE__, retval, errno);
        return;
    }
#if 0
    syslog(SYS_PRIO, "%s",
           "Address  :  0 1  2 3  4 5  6 7  8 9  A B  C D  E F 0123456789ABCDEF");
    syslog(SYS_PRIO, "%s",
           "--------   ---- ---- ---- ---- ---- ---- ---- ---- ----------------");
#endif
    for (i = 0; i < len; ) {
        /* 初期化 */
        (void)memset(logmsg, 0, sizeof(logmsg));
        (void)snprintf(logmsg, sizeof(logmsg), "%08X : ", pt);
        for (k = 0; k < 16; k++) {
            if ((i + k) >= len ) {
                (void)snprintf(tmp, sizeof(tmp),
                               "  %s", (k % 2 == 1 ? " " : ""));
                (void)strncat(logmsg, tmp,
                              (sizeof(logmsg) - strlen(logmsg) - 1));
            } else {
                (void)snprintf(tmp, sizeof(tmp), "%02x%s",
                               (unsigned int)*(p + i + k),
                               (k % 2 == 1 ? " " : ""));
                (void)strncat(logmsg, tmp,
                              (sizeof(logmsg) - strlen(logmsg) - 1));
            }
        }
        for (k = 0; (i < len) && (k < 16); k++, i++) {
            (void)snprintf(tmp, sizeof(tmp), "%c",
                           (*(p + i) < ' ' || *(p + i) >= 0x80 ?
                            '.' : *(p + i)));
            (void)strncat(logmsg, tmp, (sizeof(logmsg) - strlen(logmsg) - 1));
        }
        syslog(SYS_PRIO, "%s[%lu]: %s: %s: %s",
               filename, line, func, m_buf, logmsg);
        pt += k;
    }

    /* シスログクローズ */
    closelog();
}

/**
 * ファイルにバイナリ出力
 *
 * @param[in] prog_name プログラム名
 * @param[in] d_file ファイル名
 * @param[in] buf ダンプ出力用バッファ
 * @param[in] len 長さ
 * @return なし
 */
void dump_file(const char *prog_name, const char *d_file,
               const char *buf, const size_t len)
{
    FILE *fp = NULL; /* ファイルディスクリプタ */
    int num = 0;     /* fwrite戻り値 */
    int retval = 0;  /* 戻り値 */

    if (!buf) {
        outlog("buf[%p] len[%u]", buf, len);
        return;
    }

    /* シスログオープン */
    openlog(prog_name, LOG_PID, LOG_SYSLOG);

    fp = fopen(d_file, "wb");
    if (!fp) {
        syslog(SYS_PRIO, "%s[%d]: fopen[%p](%d)",
               __FILE__, __LINE__, fp, errno);
        return;
    }

    num = fwrite(buf, len, 1, fp);
    if (num != 1) {
        syslog(SYS_PRIO, "%s[%d]: fwrite[%p](%d)",
               __FILE__, __LINE__, fp, errno);
        return;
    }

#if 0
    retval = fflush(fp);
    if (retval == EOF) {
        syslog(SYS_PRIO, "%s[%d]: fflush[%p](%d)",
               __FILE__, __LINE__, fp, errno);
    }
#endif

    retval = fclose(fp);
    if (retval == EOF) {
        syslog(SYS_PRIO, "%s[%d]: fclose[%p](%d)",
               __FILE__, __LINE__, fp, errno);
        return;
    }

    /* シスログクローズ */
    closelog();
}


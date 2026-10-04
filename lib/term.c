/**
 * @file  lib/term.c
 * @brief ターミナル属性の取得・設定
 *
 * @author higashi
 * @date 2011-01-06 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
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

#include <stdio.h>  /* snprintf */
#include <stdlib.h> /* free */
#include <string.h> /* memcpy memset strdup strlen */
#include <unistd.h> /* STDIN_FILENO */

#include "def.h"
#include "log.h"
#include "term.h"

#define BUF_SIZE 512u /**< バッファサイズ */

/* 内部関数 */
/** ターミナル属性文字列取得 */
static char *get_termattr(const int fd, struct termios *mode);
/** バッファへの文字列追記 */
static void append_str(char *buf, const size_t size, size_t *off, const char *str);
/** モードからフラグ取得 */
static tcflag_t *mode_type_flag(const enum mode_type type, struct termios *mode);
/** モード情報構造体 */
struct _mode_info {
    const char *name;    /**< モード名 */
    enum mode_type type; /**< tcflag_t構造体種別 */
    unsigned long bits;  /**< モード設定されている */
    unsigned long mask;  /**< 未設定のとき設定されるべきビット */
};

/** モード情報 */
static const struct _mode_info mode_info[] = {
    {"parenb",  control, PARENB,  0UL   },
    {"parodd",  control, PARODD,  0UL   },
    {"cs5",     control, CS5,     CSIZE },
    {"cs6",     control, CS6,     CSIZE },
    {"cs7",     control, CS7,     CSIZE },
    {"cs8",     control, CS8,     CSIZE },
    {"hupcl",   control, HUPCL,   0UL   },
    {"cstopb",  control, CSTOPB,  0UL   },
    {"cread",   control, CREAD,   0UL   },
    {"clocal",  control, CLOCAL,  0UL   },
#ifdef CRTSCTS
    {"crtscts", control, CRTSCTS, 0UL   },
#endif
    {"ignbrk",  input,   IGNBRK,  0UL   },
    {"brkint",  input,   BRKINT,  0UL   },
    {"ignpar",  input,   IGNPAR,  0UL   },
    {"parmrk",  input,   PARMRK,  0UL   },
    {"inpck",   input,   INPCK,   0UL   },
    {"istrip",  input,   ISTRIP,  0UL   },
    {"inlcr",   input,   INLCR,   0UL   },
    {"igncr",   input,   IGNCR,   0UL   },
    {"icrnl",   input,   ICRNL,   0UL   },
    {"ixon",    input,   IXON,    0UL   },
    {"ixoff",   input,   IXOFF,   1UL   },
#ifdef IUCLC
    {"iuclc",   input,   IUCLC,   0UL   },
#endif
#ifdef IXANY
    {"ixany",   input,   IXANY,   0UL   },
#endif
#ifdef IMAXBEL
    {"imaxbel", input,   IMAXBEL, 0UL   },
#endif
#ifdef IUTF8
    {"iutf8",   input,   IUTF8,   0     },
#endif
    {"opost",   output,  OPOST,   0UL   },
#ifdef OLCUC
    {"olcuc",   output,  OLCUC,   0UL   },
#endif
#ifdef OCRNL
    {"ocrnl",   output,  OCRNL,   0UL   },
#endif
#ifdef ONLCR
    {"onlcr",   output,  ONLCR,   0UL   },
#endif
#ifdef ONOCR
    {"onocr",   output,  ONOCR,   0UL   },
#endif
#ifdef ONLRET
    {"onlret",  output,  ONLRET,  0UL   },
#endif
#ifdef OFILL
    {"ofill",   output,  OFILL,   0UL   },
#endif
#ifdef OFDEL
    {"ofdel",   output,  OFDEL,   0UL   },
#endif
#ifdef NLDLY
    {"nl1",     output,  NL1,     NLDLY },
    {"nl0",     output,  NL0,     NLDLY },
#endif
#ifdef CRDLY
    {"cr3",     output,  CR3,     CRDLY },
    {"cr2",     output,  CR2,     CRDLY },
    {"cr1",     output,  CR1,     CRDLY },
    {"cr0",     output,  CR0,     CRDLY },
#endif
#ifdef TABDLY
#  ifdef TAB3
    {"tab3",    output,  TAB3,    TABDLY},
#  endif
#  ifdef TAB2
    {"tab2",    output,  TAB2,    TABDLY},
#  endif
#  ifdef TAB1
    {"tab1",    output,  TAB1,    TABDLY},
#  endif
#  ifdef TAB0
    {"tab0",    output,  TAB0,    TABDLY},
#  endif
#  ifdef OXTABS
    {"tab0",    output,  OXTABS,  TABDLY},
#  endif
#endif
#ifdef BSDLY
    {"bs1",     output,  BS1,     BSDLY },
    {"bs0",     output,  BS0,     BSDLY },
#endif
#ifdef VTDLY
    {"vt1",     output,  VT1,     VTDLY },
    {"vt0",     output,  VT0,     VTDLY },
#endif
#ifdef FFDLY
    {"ff1",     output,  FF1,     FFDLY },
    {"ff0",     output,  FF0,     FFDLY },
#endif
    {"isig",    local,   ISIG,    0UL   },
    {"icanon",  local,   ICANON,  0UL   },
#ifdef IEXTEN
    {"iexten",  local,   IEXTEN,  0UL   },
#endif
    {"echo",    local,   ECHO,    0UL   },
    {"echoe",   local,   ECHOE,   0UL   },
    {"echok",   local,   ECHOK,   0UL   },
    {"echonl",  local,   ECHONL,  0UL   },
    {"noflsh",  local,   NOFLSH,  0UL   },
#ifdef XCASE
    {"xcase",   local,   XCASE,   0UL   },
#endif
#ifdef TOSTOP
    {"tostop",  local,   TOSTOP,  0UL   },
#endif
#ifdef ECHOPRT
    {"echoprt", local,   ECHOPRT, 0UL   },
#endif
#ifdef ECHOCTL
    {"echoctl", local,   ECHOCTL, 0UL   },
#endif
#ifdef ECHOKE
    {"echoke",  local,   ECHOKE,  0UL   },
#endif
    {NULL,      control, 0UL,     0UL   }
};

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
 */
void
sys_print_termattr(const int level,
                   const int option,
                   const char *pname,
                   const char *fname,
                   const int line,
                   const char *func,
                   int fd)
{
    struct termios mode; /* termios構造体 */
    char *result = NULL; /* 端末情報文字列 */

    (void)memset(&mode, 0, sizeof(struct termios));

    /* 端末情報を取得できなければ (端末ではない), 何も出力しない */
    result = get_termattr(fd, &mode);
    if (result == NULL)
        return;

    openlog(pname, option, SYS_FACILITY);
    syslog(level, "%s[%d]: %s: %s", fname, line, func, result);

    closelog();

    if (result != NULL)
        free(result);
    result = NULL;
}

/**
 * ターミナル属性文字列取得
 *
 * @param[in] fd ファイルディスクリプタ
 * @param[in] mode termios構造体
 * @return ターミナル属性文字列
 * @attention 戻り値ポインタは解放しなければならない
 */
static char *
get_termattr(const int fd, struct termios *mode)
{
    tcflag_t *bitsp = NULL;   /* ビット */
    unsigned long mask = 0UL; /* マスク */
    char buf[BUF_SIZE] = {0}; /* バッファ */
    char *ptr = NULL;         /* 戻り値ポインタ */
    int retval = 0;           /* 戻り値 */
    size_t off = 0u;          /* オフセット */

    dbglog("start: fd=%d", fd);

    if (fd < 0) {
        outlog("tcgetattr: fd=%d, mode=%p", fd, (const void *)mode);
        return NULL;
    }

    retval = tcgetattr(fd, mode);
    if (retval < 0)
        return NULL;

    /* 最初に "tcgetattr(" を書き込む */
    append_str(buf, sizeof(buf), &off, "tcgetattr(");

    int i = 0;
    for (i = 0; mode_info[i].name != NULL; i++) {
        bitsp = mode_type_flag(mode_info[i].type, mode);
        if (bitsp == NULL) /* 不正なモード種別 */
            continue;
        mask = (mode_info[i].mask != 0UL) ? mode_info[i].mask : mode_info[i].bits;

        if ((*bitsp & mask) == mode_info[i].bits) {
            append_str(buf, sizeof(buf), &off, mode_info[i].name);
            append_str(buf, sizeof(buf), &off, ", ");
        }
    }

    /* 末尾の ", " を削る処理 */
    if ((off >= 10u) && (buf[off - 2u] == ',') && (buf[off - 1u] == ' ')) {
        off -= 2u;
        buf[off] = '\0';
    }

    /* 閉じカッコを追加 */
    append_str(buf, sizeof(buf), &off, ")");

    ptr = strdup(buf);
    if (ptr == NULL) {
        outlog("strdup failed for buf");
        return NULL;
    }

    return ptr;
}

/**
 * バッファへの文字列追記
 *
 * 入り切らない分は, 切り捨てる. 常に, NUL 終端する (*off は, size - 1 以下).
 *
 * @param[in,out] buf バッファ
 * @param[in] size バッファのサイズ
 * @param[in,out] off 書き込み位置 (追記した長さだけ進む)
 * @param[in] str 追記する文字列
 */
static void
append_str(char *buf, const size_t size, size_t *off, const char *str)
{
    size_t len = strlen(str);       /* 追記する長さ */
    size_t room = size - 1u - *off; /* 追記できる長さ (NUL を除く) */

    if (len > room)
        len = room;
    (void)memcpy(buf + *off, str, len);
    *off += len;
    buf[*off] = '\0';
}

/**
 * モード種別からフラグ取得
 *
 * @param[in] type モード種別
 * @param[in] mode termios構造体
 * @return フラグ
 * @retval NULL 不正なモード種別
 */
static tcflag_t *
mode_type_flag(const enum mode_type type, struct termios *mode)
{
    /* 種別に対応するフラグを返す */
    switch (type) {
    case control:
        return &mode->c_cflag;
    case input:
        return &mode->c_iflag;
    case output:
        return &mode->c_oflag;
    case local:
        return &mode->c_lflag;
    default:
        return NULL;
    }
}

#ifdef UNITTEST
/**
 * 単体テスト用の関数構造体の初期化 (内部関数を, テストから呼べるようにする)
 *
 * @param[out] term 関数構造体
 */
void
test_init_term(testterm *term)
{
    term->get_termattr = get_termattr;
    term->mode_type_flag = mode_type_flag;
}
#endif

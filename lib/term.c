/**
 * @file  lib/term.c
 * @brief ターミナル属性の取得・設定
 *
 * @author higashi
 * @date 2011-01-06 higashi 新規作成
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

#include <stdio.h>  /* snprintf */
#include <stdlib.h> /* free */
#include <string.h> /* memset */
#include <unistd.h> /* STDIN_FILENO */

#include "def.h"
#include "log.h"
#include "term.h"

#define BUF_SIZE 512 /**< バッファサイズ */

/* 内部関数 */
/** ターミナル属性文字列取得 */
static char *get_termattr(const int fd, struct termios *mode);
/** モードからフラグ取得 */
static tcflag_t *mode_type_flag(const enum mode_type type,
                                struct termios *mode);
/** モード情報構造体 */
struct _mode_info {
    const char *name;    /**< モード名 */
    enum mode_type type; /**< tcflag_t構造体種別 */
    unsigned long bits;  /**< モード設定されている */
    unsigned long mask;  /**< 未設定のとき設定されるべきビット */
};

/** モード情報 */
static const struct _mode_info mode_info[] = {
    { "parenb",  control, PARENB,  0      },
    { "parodd",  control, PARODD,  0      },
    { "cs5",     control, CS5,     CSIZE  },
    { "cs6",     control, CS6,     CSIZE  },
    { "cs7",     control, CS7,     CSIZE  },
    { "cs8",     control, CS8,     CSIZE  },
    { "hupcl",   control, HUPCL,   0      },
    { "cstopb",  control, CSTOPB,  0      },
    { "cread",   control, CREAD,   0      },
    { "clocal",  control, CLOCAL,  0      },
#ifdef CRTSCTS
    { "crtscts", control, CRTSCTS, 0      },
#endif
    { "ignbrk",  input,   IGNBRK,  0      },
    { "brkint",  input,   BRKINT,  0      },
    { "ignpar",  input,   IGNPAR,  0      },
    { "parmrk",  input,   PARMRK,  0      },
    { "inpck",   input,   INPCK,   0      },
    { "istrip",  input,   ISTRIP,  0      },
    { "inlcr",   input,   INLCR,   0      },
    { "igncr",   input,   IGNCR,   0      },
    { "icrnl",   input,   ICRNL,   0      },
    { "ixon",    input,   IXON,    0      },
    { "ixoff",   input,   IXOFF,   1      },
#ifdef IUCLC
    { "iuclc",   input,   IUCLC,   0      },
#endif
#ifdef IXANY
    { "ixany",   input,   IXANY,   0      },
#endif
#ifdef IMAXBEL
    { "imaxbel", input,   IMAXBEL, 0      },
#endif
#ifdef IUTF8
    { "iutf8",   input,   IUTF8,   0      },
#endif
    { "opost",   output,  OPOST,   0      },
#ifdef OLCUC
    { "olcuc",   output,  OLCUC,   0      },
#endif
#ifdef OCRNL
    { "ocrnl",   output,  OCRNL,   0      },
#endif
#ifdef ONLCR
    { "onlcr",   output,  ONLCR,   0      },
#endif
#ifdef ONOCR
    { "onocr",   output,  ONOCR,   0      },
#endif
#ifdef ONLRET
    { "onlret",  output,  ONLRET,  0      },
#endif
#ifdef OFILL
    { "ofill",   output,  OFILL,   0      },
#endif
#ifdef OFDEL
    { "ofdel",   output,  OFDEL,   0      },
#endif
#ifdef NLDLY
    { "nl1",     output,  NL1,     NLDLY  },
    { "nl0",     output,  NL0,     NLDLY  },
#endif
#ifdef CRDLY
    { "cr3",     output,  CR3,     CRDLY  },
    { "cr2",     output,  CR2,     CRDLY  },
    { "cr1",     output,  CR1,     CRDLY  },
    { "cr0",     output,  CR0,     CRDLY  },
#endif
#ifdef TABDLY
#  ifdef TAB3
    { "tab3",    output,  TAB3,    TABDLY },
#  endif
#  ifdef TAB2
    { "tab2",    output,  TAB2,    TABDLY },
#  endif
#  ifdef TAB1
    { "tab1",    output,  TAB1,    TABDLY },
#  endif
#  ifdef TAB0
    { "tab0",    output,  TAB0,    TABDLY },
#  endif
#  ifdef OXTABS
    { "tab0",    output,  OXTABS,  TABDLY },
#  endif
#endif
#ifdef BSDLY
    { "bs1",     output,  BS1,     BSDLY  },
    { "bs0",     output,  BS0,     BSDLY  },
#endif
#ifdef VTDLY
    { "vt1",     output,  VT1,     VTDLY  },
    { "vt0",     output,  VT0,     VTDLY  },
#endif
#ifdef FFDLY
    { "ff1",     output,  FF1,     FFDLY  },
    { "ff0",     output,  FF0,     FFDLY  },
#endif
    { "isig",    local,   ISIG,    0      },
    { "icanon",  local,   ICANON,  0      },
#ifdef IEXTEN
    { "iexten",  local,   IEXTEN,  0      },
#endif
    { "echo",    local,   ECHO,    0      },
    { "echoe",   local,   ECHOE,   0      },
    { "echok",   local,   ECHOK,   0      },
    { "echonl",  local,   ECHONL,  0      },
    { "noflsh",  local,   NOFLSH,  0      },
#ifdef XCASE
    { "xcase",   local,   XCASE,   0      },
#endif
#ifdef TOSTOP
    { "tostop",  local,   TOSTOP,  0      },
#endif
#ifdef ECHOPRT
    { "echoprt", local,   ECHOPRT, 0      },
#endif
#ifdef ECHOCTL
    { "echoctl", local,   ECHOCTL, 0      },
#endif
#ifdef ECHOKE
    { "echoke",  local,   ECHOKE,  0      },
#endif
    { NULL,      control, 0,       0      }
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
    tcflag_t *bitsp = NULL; /* ビット */
    unsigned long mask = 0; /* マスク */
    char buf[BUF_SIZE];     /* バッファ */
    char *ptr = NULL;       /* 戻り値ポインタ */
    char *endp = NULL;      /* strrchr戻り値 */
    int retval = 0;         /* 戻り値 */

    dbglog("start: fd=%d", fd);

    if (fd < 0) {
        outlog("tcgetattr: fd=%d, mode=%p", fd, mode);
        return NULL;
    }

    dbglog("c_cflag=0x%x, c_iflag=0x%x, c_oflag=0x%x, c_lflag=0x%x",
           mode->c_cflag, mode->c_iflag, mode->c_oflag, mode->c_lflag);

    retval = tcgetattr(fd, mode);
    if (retval < 0)
        return NULL;

    (void)memset(buf, 0, sizeof(buf));
    (void)snprintf(buf, sizeof(buf), "tcgetattr(");
    int i;
    for (i = 0; mode_info[i].name != NULL; i++) {
        bitsp = mode_type_flag(mode_info[i].type, mode);
        mask = mode_info[i].mask ? : mode_info[i].bits;
        if ((*bitsp & mask) == mode_info[i].bits) {
            (void)strncat(buf, mode_info[i].name,
                          sizeof(buf) - strlen(buf) - 1);
            (void)strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
        }
    }

    endp = strrchr(buf, ',');
    if (endp)
        *endp = '\0';
    (void)strncat(buf, ")", sizeof(buf) - strlen(buf) - 1);

    ptr = strdup(buf);
    if (!ptr) {
        outlog("strdup: buf=%p", buf);
        return NULL;
    }

    return ptr;
}

/**
 * モード種別からフラグ取得
 *
 * @return フラグ
 */
static tcflag_t *
mode_type_flag(const enum mode_type type, struct termios *mode)
{
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
void
test_init_term(testterm *term)
{
    term->get_termattr = get_termattr;
    term->mode_type_flag = mode_type_flag;
}
#endif


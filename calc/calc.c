/**
 * @file  calc.c
 * @brief 関数電卓インタプリタ
 *
 * 式とは、項を + または - でつないだものであり、項とは因子を * または / で\n
 * つないだものであり、因子とは数または ( ) で囲んだ式である.\n
 * 再帰的下降構文解析を使用する.\n
 * 加減乗除 [+,-,*,/]、括弧 [(,)]、正負符号 [+,-]、べき乗[^]、\n
 * 関数 [abs,sqrt,sin,cos,tan,asin,acos,atan,exp,ln,log,deg,rad,n,nPr,nCr]、\n
 * 定数 [pi,e] が使用できる.
 *
 * @sa calc.h
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
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

#include <stdio.h>   /* FILE */
#include <stdlib.h>  /* realloc */
#include <string.h>  /* strndup */
#include <stdbool.h> /* bool */
#include <string.h>  /* memcpy memset */
#include <ctype.h>   /* isdigit isalpha */
#include <math.h>    /* powl */

#include "log.h"
#include "data.h"
#include "error.h"
#include "function.h"
#include "calc.h"

#define EX_ERROR    (ldfl)0 /* エラー戻り値 */

long int precision = -1;

/* 内部変数 */
static int ch = 0;      /**< 文字 */
static uchar *p = NULL; /**< 文字列先頭ポインタ */

/* 内部関数 */
/** 式 */
static ldfl expression(void);
/** 項 */
static ldfl term(void);
/** 因子 */
static ldfl factor(void);
/** 数または関数 */
static ldfl token(void);
/** 文字列を数値に変換 */
static ldfl to_numvalue(void);
/** 桁数取得 */
static long get_digit(const ldfl val, const char *fmt);
/** バッファ読込 */
static void readch(void);

/**
 * 式
 *
 * @return 値
 */
static ldfl
expression(void)
{
    ldfl x = 0; /* 値 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    x = term();
    dbglog("x[%.18Lg]", x);

    while (true) {
        if (ch == '+') {
            readch();
            x += term();
        } else if (ch == '-') {
            readch();
            x -= term();
        } else {
            break;
        }
    }

    dbglog("x[%.18Lg]", x);
    return x;
}

/**
 * 項
 *
 * @return 結果
 */
static ldfl
term(void)
{
    ldfl x = 0, y = 0; /* 値 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    x = factor();
    dbglog("x[%.18Lg]", x);

    while (true) {
        if (ch == '*') {
            readch();
            x *= factor();
        } else if (ch == '/') {
            readch();
            y = factor();
            if (y == 0) { /* ゼロ除算エラー */
                set_errorcode(E_ZERO);
                break;
            }
            x /= y;
        } else if (ch == '^') {
            readch();
            y = factor();
            x = powl(x, y);
        } else {
            break;
        }
    }
    dbglog("x[%.18Lg]", x);
    return x;
}

/**
 * 因子
 *
 * @return 値
 */
static ldfl 
factor(void)
{
    ldfl x = 0; /* 値 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    if (ch != '(')
        return token();

    readch();
    x = expression();

    if (ch != ')') { /* シンタックスエラー */
        set_errorcode(E_SYNTAX);
        return EX_ERROR;
    }
    readch();

    dbglog("x[%.18Lg]", x);
    return x;
}

/**
 * 数または関数
 *
 * @return 結果
 */
static ldfl
token(void)
{
    ldfl result = 0;                /* 結果 */
    int sign = '+';                 /* 単項+- */
    char func[MAX_FUNC_STRING + 1]; /* 関数文字列 */
    int pos = 0;                    /* 配列位置 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    (void)memset(func, 0, sizeof(func)); /* 初期化 */

    if (ch == '+' || ch == '-') { /* 単項+- */
        sign = ch;
        readch();
    }

    if (isdigit(ch)) { /* 数値 */
        result = to_numvalue();
    } else if (isalpha(ch)) { /* 文字 */
        while (isalpha(ch) && ch != '\0' && pos <= MAX_FUNC_STRING) {
            func[pos++] = ch;
            readch();
        }
        dbglog("func[%s]", func);

        result = exec_func(&value, func);

    } else { /* エラー */
        dbglog("not isdigit && not isalpha");
        set_errorcode(E_SYNTAX);
    }

    dbglog("result[%.18Lg]", result);
    return (sign == '+') ? result : -result;
}

/**
 * 文字列を数値に変換
 *
 * @return 数値
 */
static ldfl 
to_numvalue(void)
{
    ldfl x = 0, y = 1; /* 値 */

    dbglog("start");

    x = ch - '0';
    readch();
    while (isdigit(ch) && ch != '\0') { /* 整数 */
        x = (x * 10) + (ch - '0');
        dbglog("x[%.18Lg]", x);
        readch();
    }

    if (ch == '.') { /* 小数 */
        readch();
        while (isdigit(ch) && ch != '\0') {
            x += (y /= 10) * (ch - '0');
            dbglog("x[%.18Lg]", x);
            readch();
        }
    }

    check_validate(x);

    dbglog("x[%.18Lg]", x);
    return x;
}

/**
 * 桁数取得
 *
 * @param[in] val 値
 * @param[in] fmt フォーマット
 * @return 桁数
 * @retval -1 エラー
 */
static long
get_digit(ldfl val, const char *fmt)
{
    FILE *fp = NULL; /* ファイルポインタ */
    int retval = 0;  /* fclose戻り値 */
    long result = 0; /* 桁数 */

    dbglog("start");

    fp = fopen("/dev/null", "w");
    if (!fp) { /* fopen エラー */
        outlog("fopen[%p]", fp);
    } else {
        result = fprintf(fp, fmt, val);
        if (result < 0)
            outlog("fprintf[%d]", retval);

        retval = fclose(fp);
        if (retval == EOF) /* fclose エラー */
            outlog("fclose[%d]", retval);
    }

    return result;
}

/**
 * バッファ読込
 *
 * バッファから一文字読み込む.
 * 空白, タブは読み飛ばす.
 *
 * @return なし
 */
static void
readch(void)
{
    dbglog("start");

    do {
        ch = (int)*p;
        dbglog("p[%p]: %c", p, ch);
        if (ch == '\0')
            break;
        p++;
    } while (isblank(ch));
}

/**
 * 引数解析
 *
 * @param[out] val    引数
 * @param[in]  argnum 引数の数
 * @return なし
 */
void
parse_func_args(struct arg_value *val, const int argnum)
{
    dbglog("start");

    if (is_error())
        return;

    if (ch != '(') {
        set_errorcode(E_SYNTAX);
        return;
    }

    readch();
    val->x = expression();

    if (argnum == ARG_2) {
        if (ch != ',') {
            set_errorcode(E_SYNTAX);
            return;
        }
        readch();
        val->y = expression();
        dbglog("y[%.18Lg]", val->y);
    }

    if (ch != ')') {
        set_errorcode(E_SYNTAX);
        return;
    }
    readch();
}

/**
 * 入力
 *
 * @param[in] buf 文字列バッファポインタ
 * @param[in] len 文字列の長さ
 * @return 新たに領域確保された結果文字列ポインタ
 * @retval NULL エラー
 */
uchar *
input(uchar *buf, const size_t len)
{
    ldfl val = 0;               /* 値 */
    size_t slen = 0;            /* 文字数 */
    ulong digit = 0;            /* 桁数 */
    uchar *result = NULL;       /* 結果 */
    uchar *tmp = NULL;          /* 一時ポインタ */
    uchar *errormsg = NULL;     /* エラーメッセージ */
    int retval = 0;             /* 戻り値 */
    char fmt[sizeof("%.18Lg")]; /* フォーマット */

    dbglog("start");

    /* 初期値設定 */
    p = buf;  /* ポインタ設定 */

    readch();
    val = expression();
    dbglog("val[%Lg] p[%p]: %c", val, p, ch);

    check_validate(val);

    if (ch != '\0') /* エラー */
        set_errorcode(E_SYNTAX);

    if (is_error()) { /* エラー */
        errormsg = get_errormsg();
        if (!errormsg)
            return NULL;
        slen = strlen((char *)errormsg) + 1; /* 文字数 + 1 */
        result = (uchar *)strndup((char *)errormsg, slen);
        clear_error(errormsg);
        if (!result) {
            outlog("strndup[%p]", result);
            return NULL;
        }
        dbglog("result[%p] slen[%u]", result, slen);
    } else {
        /* フォーマット設定 */
        if (precision == -1) /* デフォルト */
            precision = DEFAULT_PREC;
        retval = snprintf(fmt, sizeof(fmt), "%s%ld%s",
                          "%.", precision + 1, "Lg");
        if (retval < 0) {
            outlog("snprintf[%d]", retval);
            return NULL;
        }
        dbglog("fmt[%s]", fmt);
        /* 桁数取得 */
        digit = get_digit(val, fmt);
        dbglog("digit[%lu]", digit);
        if (digit < 0) {
            outlog("digit[%lu]", digit);
            return NULL;
        }

        digit += 1; /* 桁数 + 1 */
        /* メモリ確保 */
        tmp = (uchar *)calloc(digit, sizeof(uchar));
        if (!tmp) {
            outlog("calloc[%p]", tmp);
            return NULL;
        }
        result = tmp;
        /* 値を文字列に変換 */
        retval = snprintf((char *)result, digit, fmt, val);
        if (retval < 0) {
            outlog("snprintf[%d]: result[%p] digit[%lu]",
                   retval, result, digit);
            return NULL;
        }
        dbglog("result[%s] val[%.18Lg] digit[%lu]", result, val, digit);
    }
    return result;
}


/**
 * @file  calc/calc.c
 * @brief 関数電卓インタプリタ
 *
 * 式とは、項を + または - でつないだものであり、項とは因子を * または / で\n
 * つないだものであり、因子とは数または ( ) で囲んだ式である.\n
 * 再帰的下降構文解析を使用する.\n
 * 加減乗除 [+,-,*,/]、括弧 [(,)]、正負符号 [+,-]、べき乗[^]、\n
 * 関数 [abs,sqrt,sin,cos,tan,asin,acos,atan,exp,ln,log,deg,rad,n,nPr,nCr]、\n
 * 定数 [pi,e] が使用できる.
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
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

#include <stdio.h>    /* fprintf snprintf FILE */
#include <string.h>   /* memcpy memset */
#include <ctype.h>    /* isdigit isalpha */
#include <stdarg.h>   /* va_list va_arg */
#ifdef _DEBUG
#  include <limits.h> /* INT_MAX */
#endif /* _DEBUG */

#include "timer.h"
#include "log.h"
#include "memfree.h"
#include "func.h"
#include "error.h"
#include "calc.h"

/* 外部変数 */
bool g_tflag = false;              /**< tオプションフラグ */

/* 内部変数 */
static const dbl EX_ERROR = 0.0;   /**< エラー戻り値 */
static long digit = DEFAULT_DIGIT; /**< 桁数 */

/* 内部関数 */
/** バッファ読込 */
static void readch(calcinfo *calc);
/** 式 */
static dbl expression(calcinfo *calc);
/** 項 */
static dbl term(calcinfo *calc);
/** 因子 */
static dbl factor(calcinfo *calc);
/** 数または関数 */
static dbl token(calcinfo *calc);
/** 文字列を数値に変換 */
static dbl number(calcinfo *calc);
/** 文字数取得 */
static int get_strlen(const dbl val, const char *fmt);

/**
 * 計算結果
 *
 * @param[in] calc calcinfo構造体
 * @param[in] expr 式
 * @return 新たに領域確保された結果文字列ポインタ
 * @retval NULL エラー
 * @attention destroy_answerを必ず呼ぶこと.
 */
uchar *
create_answer(calcinfo *calc, const uchar *expr)
{
    dbl val = 0.0;     /* 値 */
    size_t length = 0; /* 文字数 */
    int retval = 0;    /* 戻り値 */
    uint start = 0;    /* タイマ開始 */

    dbglog("start");

    calc->ptr = (uchar *)expr; /* 走査用ポインタ */
    dbglog("ptr=%p", calc->ptr);

    /* フォーマット設定 */
    retval = snprintf(calc->fmt, sizeof(calc->fmt),
                      "%s%ld%s", "%.", digit, "g");
    if (retval < 0) {
        outlog("snprintf");
        return NULL;
    }
    dbglog("fmt=%s", calc->fmt);

    readch(calc);

    if (g_tflag)
        start_timer(&start);

    val = expression(calc);
    dbglog(calc->fmt, val);
    dbglog("ptr=%p, ch=%c", calc->ptr, calc->ch);

    check_validate(calc, val);
    if (calc->ch != '\0') /* エラー */
        set_errorcode(calc, E_SYNTAX);

    if (g_tflag) {
        uint calc_time = stop_timer(&start);
        print_timer(calc_time);
    }

    if (is_error(calc)) { /* エラー */
        calc->answer = get_errormsg(calc);
        clear_error(calc);
        if (!calc->answer)
            return NULL;
        dbglog("answer=%p, length=%zu", calc->answer, length);
    } else {
        /* 文字数取得 */
        retval = get_strlen(val, calc->fmt);
        if (retval <= 0) { /* エラー */
            outlog("get_strlen=%d", retval);
            return NULL;
        }
        dbglog("get_strlen=%d, INT_MAX=%d", retval, INT_MAX);
        length = (size_t)retval + 1; /* 文字数 + 1 */

        /* メモリ確保 */
        calc->answer = (uchar *)malloc(length * sizeof(uchar));
        if (!calc->answer) {
            outlog("malloc: length=%zu", length);
            return NULL;
        }
        (void)memset(calc->answer, 0, length * sizeof(uchar));

        /* 値を文字列に変換 */
        retval = snprintf((char *)calc->answer, length, calc->fmt, val);
        if (retval < 0) {
            outlog("snprintf: answer=%p, length=%zu", calc->answer, length);
            return NULL;
        }
        dbglog(calc->fmt, val);
        dbglog("answer=%s, length=%zu", calc->answer, length);
    }
    return calc->answer;
}

/**
 * メモリ解放
 *
 * @param[in] calc calcinfo構造体ポインタ
 * @return なし
 */
void
destroy_answer(void *calc)
{
    calcinfo *ptr = (calcinfo *)calc;
    dbglog("start: result=%p", ptr->answer);
    memfree((void **)&ptr->answer, NULL);
}

/**
 * 引数解析
 *
 * @param[in] calc calcinfo構造体
 * @param[out] x 値
 * @param[out] ... 可変引数
 * @return なし
 * @attention 最後の引数はNULLにすること.
 */
void
parse_func_args(calcinfo *calc, dbl *x, ...)
{
    dbl *val = NULL; /* 値 */
    va_list ap;      /* va_list */

    dbglog("start");

    if (is_error(calc))
        return;

    if (calc->ch != '(') {
        set_errorcode(calc, E_SYNTAX);
        return;
    }

    readch(calc);
    *x = expression(calc);
    dbglog(calc->fmt, *x);

    va_start(ap, x);

    while ((val = va_arg(ap, dbl *)) != NULL) {
        if (calc->ch != ',') {
            set_errorcode(calc, E_SYNTAX);
            va_end(ap);
            return;
        }
        readch(calc);
        *val = expression(calc);
        dbglog(calc->fmt, *val);
    }

    va_end(ap);

    if (calc->ch != ')') {
        set_errorcode(calc, E_SYNTAX);
        return;
    }
    readch(calc);
}

/**
 * 桁数設定
 *
 * @return なし
 */
void
set_digit(long dgt)
{
    digit = dgt;
}

/**
 * バッファ読込
 *
 * バッファから一文字読み込む.
 * 空白, タブは読み飛ばす.
 *
 * @param[in] calc calcinfo構造体
 * @return なし
 */
static void
readch(calcinfo *calc)
{
    dbglog("start");

    do {
        calc->ch = (int)*calc->ptr;
        dbglog("ptr=%p, ch=%c", calc->ptr, calc->ch);
        if (calc->ch == '\0')
            break;
        calc->ptr++;
    } while (isblank(calc->ch));
}

/**
 * 式
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static dbl
expression(calcinfo *calc)
{
    dbl x = 0.0; /* 値 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    x = term(calc);
    dbglog(calc->fmt, x);

    while (true) {
        if (calc->ch == '+') {
            readch(calc);
            x += term(calc);
        } else if (calc->ch == '-') {
            readch(calc);
            x -= term(calc);
        } else {
            break;
        }
    }

    dbglog(calc->fmt, x);
    return x;
}

/**
 * 項
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static dbl
term(calcinfo *calc)
{
    dbl x = 0.0, y = 0.0; /* 値 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    x = factor(calc);
    dbglog(calc->fmt, x);

    while (true) {
        if (calc->ch == '*') {
            readch(calc);
            x *= factor(calc);
        } else if (calc->ch == '/') {
            readch(calc);
            y = factor(calc);
            if (y == 0) { /* ゼロ除算エラー */
                set_errorcode(calc, E_DIVBYZERO);
                return EX_ERROR;
            }
            x /= y;
        } else if (calc->ch == '^') {
            readch(calc);
            y = factor(calc);
            x = get_pow(calc, x, y);
        } else {
            break;
        }
    }
    dbglog(calc->fmt, x);
    return x;
}

/**
 * 因子
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static dbl
factor(calcinfo *calc)
{
    dbl x = 0.0; /* 値 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    if (calc->ch != '(')
        return token(calc);

    readch(calc);
    x = expression(calc);

    if (calc->ch != ')') { /* シンタックスエラー */
        set_errorcode(calc, E_SYNTAX);
        return EX_ERROR;
    }
    readch(calc);

    dbglog(calc->fmt, x);
    return x;
}

/**
 * 数または関数
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static dbl
token(calcinfo *calc)
{
    dbl result = 0.0;               /* 結果 */
    int sign = '+';                 /* 単項+- */
    char func[MAX_FUNC_STRING + 1]; /* 関数文字列 */
    int pos = 0;                    /* 配列位置 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    /* 初期化 */
    (void)memset(func, 0, sizeof(func));

    if (calc->ch == '+' || calc->ch == '-') { /* 単項+- */
        sign = calc->ch;
        readch(calc);
    }

    if (isdigit(calc->ch)) { /* 数値 */
        result = number(calc);
    } else if (isalpha(calc->ch)) { /* 関数 */
        while (isalpha(calc->ch) && calc->ch != '\0' &&
               pos <= MAX_FUNC_STRING) {
            func[pos++] = calc->ch;
            readch(calc);
        }
        dbglog("func=%s", func);

        result = exec_func(calc, func);

    } else { /* エラー */
        dbglog("ch=%c", calc->ch);
        set_errorcode(calc, E_SYNTAX);
    }

    dbglog(calc->fmt, result);
    return (sign == '+') ? result : -result;
}

/**
 * 文字列を数値に変換
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static dbl
number(calcinfo *calc)
{
    dbl x = 0.0, y = 1.0; /* 値 */

    dbglog("start");

    x = calc->ch - '0';
    while (readch(calc), isdigit(calc->ch)) /* 整数 */
        x = (x * 10) + (calc->ch - '0');
    dbglog(calc->fmt, x);

    if (calc->ch == '.') { /* 小数 */
        while (readch(calc), isdigit(calc->ch))
            x += (y /= 10) * (calc->ch - '0');
    }
    dbglog(calc->fmt, x);

    check_validate(calc, x);

    return x;
}

/**
 * 文字数取得
 *
 * @param[in] val 値
 * @param[in] fmt フォーマット
 * @return 文字数
 * @retval  0 fopenエラー
 * @retval -1 fprintfエラー
 */
static int
get_strlen(const dbl val, const char *fmt)
{
    FILE *fp = NULL; /* ファイルポインタ */
    int retval = 0;  /* fclose戻り値 */
    int length = 0;  /* 文字数 */

    dbglog("start: fmt=%s", fmt);
    dbglog(fmt, val);

    fp = fopen("/dev/null", "w");
    if (!fp) { /* fopen エラー */
        outlog("fopen");
    } else {
        length = fprintf(fp, fmt, val);
        if (length < 0)
            outlog("fprintf: fp=%p, fmt=%s, val=%g", fp, fmt, val);

        retval = fclose(fp);
        if (retval == EOF) /* fclose エラー */
            outlog("fclose: fp=%p", fp);
    }

    dbglog("length=%d", length);
    return length;
}

#ifdef UNITTEST
void
test_init_calc(testcalc *calc)
{
    calc->expression = expression;
    calc->term = term;
    calc->factor = factor;
    calc->token = token;
    calc->number = number;
    calc->get_strlen = get_strlen;
    calc->readch = readch;
}
#endif /* UNITTEST */


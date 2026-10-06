/**
 * @file  calc/calc.c
 * @brief 関数電卓インタプリタ
 *
 * 式とは, 項を + または - でつないだものであり, 項とは因子を * または / で\n
 * つないだものであり, 因子とは数または ( ) で囲んだ式である.\n
 * 再帰的下降構文解析を使用する.\n
 * 加減乗除 [+,-,*,/], 括弧 [(,)], 正負符号 [+,-], べき乗[^], \n
 * 関数 [abs,sqrt,sin,cos,tan,asin,acos,atan,exp,ln,log,deg,rad,n,nPr,nCr], \n
 * 定数 [pi,e] が使用できる.
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
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

#include <stdio.h>  /* fprintf snprintf FILE */
#include <string.h> /* memcpy memset */
#include <stdlib.h> /* malloc */
#include <ctype.h>  /* isdigit isalpha */
#include <stdarg.h> /* va_list va_arg */
#include <math.h>   /* fpclassify FP_ZERO */
#ifdef _DEBUG
#  include <limits.h> /* INT_MAX */
#endif                /* _DEBUG */

#include "timer.h"
#include "log.h"
#include "memfree.h"
#include "func.h"
#include "error.h"
#include "calc.h"

/* 外部変数 */
bool g_tflag = false; /**< tオプションフラグ */

/* 内部変数 */
static const double EX_ERROR = 0.0; /**< エラー戻り値 */
static long digit = DEFAULT_DIGIT;  /**< 桁数 */

/* 内部関数 */
static void readch(calcinfo *calc);
static double expression(calcinfo *calc);
static double term(calcinfo *calc);
static double unary(calcinfo *calc);
static double power(calcinfo *calc);
static double factor(calcinfo *calc);
static double token(calcinfo *calc);
static double number(calcinfo *calc);
static int get_strlen(const double val, const char *fmt);

/**
 * @brief 計算結果
 *
 * @param[in] calc calcinfo構造体
 * @param[in] expr 式
 * @return 新たに領域確保された結果文字列ポインタ
 * @retval NULL エラー
 * @attention destroy_answerを必ず呼ぶこと.
 */
unsigned char *
create_answer(calcinfo *calc, const unsigned char *expr)
{
    double val = 0.0;        /* 値 */
    size_t length = 0U;      /* 文字数 */
    int retval = 0;          /* 戻り値 */
    unsigned int start = 0U; /* タイマ開始 */

    dbglog("start");

    calc->ptr = expr; /* 走査用ポインタ */
    dbglog("ptr=%p", calc->ptr);

    /* フォーマット設定 */
    retval = snprintf(calc->fmt, sizeof(calc->fmt), "%s%ld%s", "%.", digit, "g");
    if (retval < 0) {
        outlog("snprintf");
        return NULL;
    }
    dbglog("fmt=%s", calc->fmt);

    readch(calc);

    if (g_tflag)
        start_timer(&start);

    val = expression(calc);
    dbglog("%.*g", (int)digit, val);
    dbglog("ptr=%p, ch=%c", calc->ptr, calc->ch);

    check_validate(calc, val);
    if (calc->ch != '\0') /* エラー */
        set_errorcode(calc, E_SYNTAX);

    if (g_tflag) {
        unsigned int calc_time = stop_timer(&start);
        print_timer(calc_time);
    }

    if (is_error(calc)) { /* エラー */
        calc->answer = get_errormsg(calc);
        clear_error(calc);
        if (calc->answer == NULL)
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
        length = (size_t)retval + 1U; /* 文字数 + 1 */

        /* メモリ確保 */
        calc->answer = (unsigned char *)malloc(length * sizeof(unsigned char));
        if (calc->answer == NULL) {
            outlog("malloc: length=%zu", length);
            return NULL;
        }
        (void)memset(calc->answer, 0, length * sizeof(unsigned char));

        /* 値を文字列に変換 */
        /* 書式 ("%.<桁数>g") は, 桁数 (digit) から作るので文字列リテラルではない */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        retval = snprintf((char *)calc->answer, length, calc->fmt, val);
#pragma GCC diagnostic pop
        if (retval < 0) {
            outlog("snprintf: answer=%p, length=%zu", calc->answer, length);
            return NULL;
        }
        dbglog("%.*g", (int)digit, val);
        dbglog("answer=%s, length=%zu", calc->answer, length);
    }
    return calc->answer;
}

/**
 * @brief メモリ解放
 *
 * @param[in] calc calcinfo構造体ポインタ
 */
void
destroy_answer(void *calc)
{
    calcinfo *ptr = (calcinfo *)calc; /* calcinfo構造体 */
    dbglog("start: result=%p", ptr->answer);
    memfree(&ptr->answer, NULL);
}

/**
 * @brief 引数解析
 *
 * @param[in] calc calcinfo構造体
 * @param[out] x 値
 * @param[out] ... 可変引数
 * @attention 最後の引数はNULLにすること.
 */
void
parse_func_args(calcinfo *calc, double *x, ...)
{
    double *val = NULL; /* 値 */
    va_list ap;         /* va_list */

    dbglog("start");

    if (is_error(calc))
        return;

    /* 引数は '(' で始まる */
    if (calc->ch != '(') {
        set_errorcode(calc, E_SYNTAX);
        return;
    }

    /* 1 つ目の引数 */
    readch(calc);
    *x = expression(calc);
    dbglog("%.*g", (int)digit, *x);

    va_start(ap, x);

    /* 2 つ目以降の引数は',' で区切られる */
    val = va_arg(ap, double *);
    while (val != NULL) {
        if (calc->ch != ',') {
            set_errorcode(calc, E_SYNTAX);
            va_end(ap);
            return;
        }
        readch(calc);
        *val = expression(calc);
        dbglog("%.*g", (int)digit, *val);
        val = va_arg(ap, double *);
    }

    va_end(ap);

    /* 引数は ')' で終わる */
    if (calc->ch != ')') {
        set_errorcode(calc, E_SYNTAX);
        return;
    }
    readch(calc);
}

/**
 * @brief 桁数設定
 *
 * @param[in] dgt 有効桁数
 */
void
set_digit(long dgt)
{
    digit = dgt;
}

#ifdef UNITTEST
/**
 * @brief 単体テスト用の関数構造体の初期化 (内部関数をテストから呼べるようにする)
 *
 * @param[out] calc 関数構造体
 */
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

/**
 * バッファ読込
 *
 * バッファから一文字読み込む.
 * 空白, タブは読み飛ばす.
 *
 * @param[in] calc calcinfo構造体
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
    } while (isblank(calc->ch) != 0);
}

/**
 * 式
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
expression(calcinfo *calc)
{
    double x = 0.0; /* 値 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    /* 最初の項 */
    x = term(calc);
    dbglog("%.*g", (int)digit, x);

    /* 続く項を'+' '-' で, 左から順に加減算する */
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

    dbglog("%.*g", (int)digit, x);
    return x;
}

/**
 * 項
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
term(calcinfo *calc)
{
    double x = 0.0, y = 0.0; /* 値 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    x = unary(calc);
    dbglog("%.*g", (int)digit, x);

    while (true) {
        if (calc->ch == '*') {
            readch(calc);
            x *= unary(calc);
        } else if (calc->ch == '/') {
            readch(calc);
            y = unary(calc);
            if (fpclassify(y) == FP_ZERO) { /* ゼロ除算エラー */
                set_errorcode(calc, E_DIVBYZERO);
                return EX_ERROR;
            }
            x /= y;
        } else {
            break;
        }
    }
    dbglog("%.*g", (int)digit, x);
    return x;
}

/**
 * 単項
 * 符号はべき乗より, 弱く結合する. (-2^2 は -(2^2))
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
unary(calcinfo *calc)
{
    double x = 0.0; /* 値 */
    int sign = '+'; /* 単項+- */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    if ((calc->ch == '+') || (calc->ch == '-')) {
        sign = calc->ch;
        readch(calc);
    }
    x = power(calc);
    return ((sign == '+') ? x : -x);
}

/**
 * べき乗
 * 乗除算より, 強く結合する. 左から右に結合する. (2^3^2 は (2^3)^2)
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
power(calcinfo *calc)
{
    double x = 0.0, y = 0.0; /* 値 */

    dbglog("start");

    /* unary() がエラー状態を確認してから呼ぶので, ここでは確認しない */
    x = factor(calc);
    dbglog("%.*g", (int)digit, x);

    while (calc->ch == '^') {
        readch(calc);
        y = factor(calc); /* 指数の符号は token() が処理する (2^-1) */
        x = get_pow(calc, x, y);
    }
    dbglog("%.*g", (int)digit, x);
    return x;
}

/**
 * 因子
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
factor(calcinfo *calc)
{
    double x = 0.0; /* 値 */

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

    dbglog("%.*g", (int)digit, x);
    return x;
}

/**
 * 数または関数
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
token(calcinfo *calc)
{
    double result = 0.0;             /* 結果 */
    int sign = '+';                  /* 単項+- */
    char func[MAX_FUNC_STRING + 1U]; /* 関数文字列 */
    unsigned int pos = 0U;           /* 配列位置 */

    dbglog("start");

    if (is_error(calc))
        return EX_ERROR;

    /* 初期化 */
    (void)memset(func, 0, sizeof(func));

    if ((calc->ch == '+') || (calc->ch == '-')) { /* 単項+- */
        sign = calc->ch;
        readch(calc);
    }

    if (isdigit(calc->ch) != 0) { /* 数値 */
        result = number(calc);
    } else if (isalpha(calc->ch) != 0) { /* 関数 */
        while ((isalpha(calc->ch) != 0) && (calc->ch != '\0') && (pos < MAX_FUNC_STRING)) {
            func[pos++] = (char)calc->ch;
            readch(calc);
        }
        dbglog("func=%s", func);

        result = exec_func(calc, func);

    } else { /* エラー */
        dbglog("ch=%c", calc->ch);
        set_errorcode(calc, E_SYNTAX);
    }

    dbglog("%.*g", (int)digit, result);
    return ((sign == '+') ? result : -result);
}

/**
 * 文字列を数値に変換
 *
 * @param[in] calc calcinfo構造体
 * @return 値
 */
static double
number(calcinfo *calc)
{
    double x = 0.0, y = 1.0; /* 値 */

    dbglog("start");

    x = calc->ch - '0';
    readch(calc);
    while (isdigit(calc->ch) != 0) { /* 整数 */
        x = (x * 10) + (calc->ch - '0');
        readch(calc);
    }
    dbglog("%.*g", (int)digit, x);

    if (calc->ch == '.') { /* 小数 */
        readch(calc);
        while (isdigit(calc->ch) != 0) {
            y /= 10.0;
            x += y * (calc->ch - '0');
            readch(calc);
        }
    }
    dbglog("%.*g", (int)digit, x);

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
get_strlen(const double val, const char *fmt)
{
    int retval = 0; /* 戻り値 */

    /* 書式 ("%.<桁数>g") は, 桁数 (digit) から作るので文字列リテラルではない */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    retval = snprintf(NULL, 0U, fmt, val);
#pragma GCC diagnostic pop
    return retval;
}

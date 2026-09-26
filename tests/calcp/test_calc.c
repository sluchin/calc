/**
 * @file  tests/calcp/test_calc.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-09-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
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

#include <stdlib.h> /* exit */
#include <stdarg.h> /* va_list */
#include <stdbool.h> /* bool */
#include <string.h> /* strcmp strncmp */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "error.h"
#include "memfree.h"
#include "calc.h"
#include "helper.h"

DEFINE_FFF_GLOBALS;

/* strdup() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(char *, strdup, const char *);
TEST_PASSTHROUGH(char *, strdup, (const char *str), (str))

/*
 * snprintf() と malloc() は, ほとんどの関数が使うので, FFF のモックにはせず,
 * 必要なときだけ失敗させる (FFF のモックは, main() より前の呼び出しでも使われる).
 */
/** snprintf() を失敗させる呼び出しの種類 */
enum snprintf_target {
    SNPRINTF_NONE,   /**< 失敗させない */
    SNPRINTF_FORMAT, /**< 書式の設定 ("%s%ld%s") */
    SNPRINTF_STRLEN, /**< 文字数の取得 (get_strlen: 出力先が NULL) */
    SNPRINTF_ANSWER  /**< 値の文字列への変換 ("%.<桁数>g") */
};
static enum snprintf_target fail_snprintf = SNPRINTF_NONE; /**< 失敗させる種類 */
/**
 * snprintf() の置き換え
 * デバッグビルドでは, dbglog() (ログ出力) も snprintf() を呼ぶので, 呼び出しの回数ではなく,
 * 呼び出しの種類で, 失敗させる.
 *
 * @param[out] str 出力先
 * @param[in] size サイズ
 * @param[in] format 書式
 * @return 出力した文字数. 注入した失敗のときは, -1
 */
int
snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int retval = 0;
    bool fail = false; /* 失敗させるか */

    switch (fail_snprintf) {
    case SNPRINTF_FORMAT:
        fail = !strcmp(format, "%s%ld%s");
        break;
    case SNPRINTF_STRLEN:
        fail = (str == NULL && size == 0);
        break;
    case SNPRINTF_ANSWER:
        fail = (str != NULL && !strncmp(format, "%.", 2));
        break;
    default:
        break;
    }
    if (fail) {
        fail_snprintf = SNPRINTF_NONE;
        errno = EIO;
        return -1;
    }
    va_start(ap, format);
    retval = vsnprintf(str, size, format, ap);
    va_end(ap);
    return retval;
}

extern void *__libc_malloc(size_t size);
static size_t fail_malloc_size = 0; /**< 失敗させる malloc() のサイズ (0 は無効) */
static int fail_malloc_count = 0;   /**< 失敗させる回数 */
/**
 * malloc() の置き換え
 *
 * @param[in] size サイズ
 * @return 確保したメモリ. 指定したサイズのときは, 失敗 (NULL)
 */
void *
malloc(size_t size)
{
    if (fail_malloc_count > 0 && size == fail_malloc_size) {
        fail_malloc_count--;
        errno = ENOMEM;
        return NULL;
    }
    return __libc_malloc(size);
}

/*
 * malloc() のあとに memset(0) するコードは, 最適化で calloc() になるので, calloc() も
 * 同じように置き換える.
 */
extern void *__libc_calloc(size_t nmemb, size_t size);
/**
 * calloc() の置き換え
 *
 * @param[in] nmemb 要素数
 * @param[in] size 要素のサイズ
 * @return 確保したメモリ. 指定したサイズのときは, 失敗 (NULL)
 */
void *
calloc(size_t nmemb, size_t size)
{
    if (fail_malloc_count > 0 && nmemb * size == fail_malloc_size) {
        fail_malloc_count--;
        errno = ENOMEM;
        return NULL;
    }
    return __libc_calloc(nmemb, size);
}

/* プロトタイプ */
/** create_answer() 関数テスト (失敗) */
TEST test_answer_failure(void);
/** 式を解析する関数テスト (エラー状態) */
TEST test_calc_error_state(void);
/** parse_func_args() 関数テスト (失敗) */
TEST test_parse_func_args_failure(void);
/** create_answer() 関数テスト (処理時間の表示) */
TEST test_answer_timer(void);
/** 四則演算テスト */
TEST test_answer_four(void);
/** 関数テスト */
TEST test_answer_func(void);
/** 四則演算と関数の組み合わせテスト */
TEST test_answer_four_func(void);
/** 関数エラー時テスト */
TEST test_answer_error(void);
/** parse_func_args() 関数テスト */
TEST test_parse_func_args(void);
/** set_digit() 関数テスト */
TEST test_set_digit(void);
/** readch() 関数テスト */
TEST test_readch(void);
/** expression() 関数テスト */
TEST test_expression(void);
/** term() 関数テスト */
TEST test_term(void);
/** factor() 関数テスト */
TEST test_factor(void);
/** token() 関数テスト */
TEST test_token(void);
/** number() 関数テスト */
TEST test_number(void);
/** get_strlen() 関数テスト */
TEST test_get_strlen(void);

/* 内部変数 */
static testcalc st_calc; /**< 関数構造体 */

/* 内部関数 */
/** バッファセット */
static void exec_calc(calcinfo *calc, const char *str);

/** テストデータ構造体(answer char) */
struct test_data_char {
    char expr[MAX_STRING];
    char answer[MAX_STRING];
};

/** テストデータ構造体(answer double) */
struct test_data_double {
    char expr[MAX_STRING];
    double answer;
    ER errorcode;
};

/** 四則演算テスト用データ */
static const struct test_data_char four_data [] = {
    { "(105+312)+2*(5-3)", "421" },
    { "(105+312)+2/(5-3)", "418" },
    { "1+2*(5-3)",         "5"   },
    { "1+2/(5-3)",         "2"   },
    { "-(1+2)",            "-3"  },
    { "2*-(3)",            "-6"  },
    { "+(1+2)",            "3"   },
    { "2*3^2",             "18"  },
    { "4/2^2",             "1"   },
    { "10-2^2",            "6"   },
    { "-2^2",              "-4"  },
    { "2^-1",              "0.5" },
    { "2^3^2",             "64"  }
};

/** 関数テスト用データ */
static const struct test_data_char func_data [] = {
    { "pi",        "3.14159265359"   },
    { "e",         "2.71828182846"   },
    { "abs(-2)",   "2"               },
    { "sqrt(2)",   "1.41421356237"   },
    { "sin(2)" ,   "0.909297426826"  },
    { "cos(2)",    "-0.416146836547" },
    { "tan(2)",    "-2.18503986326"  },
    { "asin(0.5)", "0.523598775598"  },
    { "acos(0.5)", "1.0471975512"    },
    { "atan(0.5)", "0.463647609001"  },
    { "exp(2)" ,   "7.38905609893"   },
    { "ln(2)",     "0.69314718056"   },
    { "log(2)",    "0.301029995664"  },
    { "deg(2)",    "114.591559026"   },
    { "rad(2)",    "0.0349065850399" },
    { "n(10)",     "3628800"         },
    { "nPr(5,2)",  "20"              },
    { "nCr(5,2)",  "10"              },
    { "n(170)",    "7.25741561531e+306" },
    { "exp(-1000)", "0"              },
    { "10^-400",   "0"               }
};

/** 四則演算と関数の組み合わせテスト用データ */
static const struct test_data_char four_func_data [] = {
    { "5*pi",        "15.7079632679"  },
    { "pi*5",        "15.7079632679"  },
    { "5*e",         "13.5914091423"  },
    { "e*5",         "13.5914091423"  },
    { "5*abs(-2)",   "10"             },
    { "abs(-2)*5",   "10"             },
    { "5*sqrt(2)",   "7.07106781187"  },
    { "sqrt(2)*5",   "7.07106781187"  },
    { "5*sin(2)",    "4.54648713413"  },
    { "sin(2)*5",    "4.54648713413"  },
    { "5*cos(2)",    "-2.08073418274" },
    { "cos(2)*5",    "-2.08073418274" },
    { "5*tan(2)",    "-10.9251993163" },
    { "tan(2)*5",    "-10.9251993163" },
    { "2*asin(0.5)", "1.0471975512"   },
    { "asin(0.5)*2", "1.0471975512"   },
    { "2*acos(0.5)", "2.09439510239"  },
    { "acos(0.5)*2", "2.09439510239"  },
    { "5*atan(0.5)", "2.318238045"    },
    { "atan(0.5)*5", "2.318238045"    },
    { "5*exp(2)",    "36.9452804947"  },
    { "exp(2)*5",    "36.9452804947"  },
    { "5*ln(2)",     "3.4657359028"   },
    { "ln(2)*5",     "3.4657359028"   },
    { "5*log(2)",    "1.50514997832"  },
    { "log(2)*5",    "1.50514997832"  },
    { "5*deg(2)",    "572.957795131"  },
    { "deg(2)*5",    "572.957795131"  },
    { "5*rad(2)",    "0.174532925199" },
    { "rad(2)*5",    "0.174532925199" },
    { "5*n(10)",     "18144000"       },
    { "n(10)*5",     "18144000"       },
    { "5*nPr(5,2)",  "100"            },
    { "nPr(5,2)*5",  "100"            },
    { "5*nCr(5,2)",  "50"             },
    { "nCr(5,2)*5",  "50"             }
};

/** 関数エラー時テスト用データ */
static const struct test_data_char error_data [] = {
    { "5/0",        "Divide by zero."       },
    { "sin(5",      "Syntax error."         },
    { "nCr(5)",     "Syntax error."         },
    { "nofunc(5)",  "Function not defined." },
    { "n(0.5)",     "NaN."                  },
    { "nPr(-1,-2)", "NaN."                  },
    { "nPr(1,-2)",  "NaN."                  },
    { "nPr(3,5)",   "NaN."                  },
    { "nCr(-1,-2)", "NaN."                  },
    { "nCr(1,-2)",  "NaN."                  },
    { "nCr(3,5)",   "NaN."                  },
    { "sqrt(-5)",   "NaN."                  },
    { "10^1000000", "Infinity."             },
    { "n(5000)",    "Infinity."             },
    { "n(-5000)",   "Infinity."             },
    { "n(171)",     "Infinity."             },
    { "n(99999999999999999999)", "Infinity." },
    { "nPr(99999999999999999999,1)", "Infinity." },
    { "nCr(99999999999999999999,1)", "Infinity." }
};

/** expression() 関数テスト用データ */
static const struct test_data_double expression_data [] = {
    { "5+7", 12, E_NONE },
    { "5-1",  4, E_NONE }
};

/** term() 関数テスト用データ */
static const struct test_data_double term_data [] = {
    { "5*7", 35,   E_NONE      },
    { "6/2",  3,   E_NONE      },
    { "6^2", 36,   E_NONE      },
    { "6/0",  0.0, E_DIVBYZERO }
};

/** factor() 関数テスト用データ */
static const struct test_data_double factor_data [] = {
    { "(5+4)", 9,   E_NONE   },
    { "(5+4",  0.0, E_SYNTAX }
};

/** token() 関数テスト用データ */
static const struct test_data_double token_data [] = {
    { "+54321",    54321,   E_NONE   },
    { "-54321",   -54321,   E_NONE   },
    { "54231",     54231,   E_NONE   },
    { "nCr(5,2)",     10,   E_NONE   },
    { "テスト",        0.0, E_SYNTAX }
};

/** number() 関数テスト用データ */
static const struct test_data_double number_data [] = {
    { "54321",  54321,     E_NONE },
    { "543.21",   543.21,  E_NONE },
};

/**
 * 初期化処理
 *
 * @return なし
 */
static void
startup(void)
{
    (void)memset(&st_calc, 0, sizeof(testcalc));
    test_init_calc(&st_calc);
}

/**
 * 四則演算テスト
 *
 * @return なし
 */
TEST
test_answer_four(void)
{
    calcinfo calc; /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(four_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, four_data[i].expr);

        TEST_ASSERT_STR_MSG(four_data[i].answer, (char *)calc.answer, "%s=%s",
                                            four_data[i].expr,
                                            four_data[i].answer);
        destroy_answer(&calc);
        memfree((void **)&calc, NULL);
    }
    PASS();
}

/**
 * 関数テスト
 *
 * @return なし
 */
TEST
test_answer_func(void)
{
    calcinfo calc; /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(func_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, func_data[i].expr);

        TEST_ASSERT_STR_MSG(func_data[i].answer, (char *)calc.answer, "%s=%s",
                                            func_data[i].expr,
                                            func_data[i].answer);
        destroy_answer(&calc);
    }
    PASS();
}

/**
 * 四則演算と関数の組み合わせテスト
 *
 * @return なし
 */
TEST
test_answer_four_func(void)
{
    calcinfo calc; /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(four_func_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, four_func_data[i].expr);

        TEST_ASSERT_STR_MSG(four_func_data[i].answer, (char *)calc.answer, "%s=%s",
                                            four_func_data[i].expr,
                                            four_func_data[i].answer);
        destroy_answer(&calc);
    }
    PASS();
}

/**
 * 関数エラー時テスト
 *
 * @return なし
 */
TEST
test_answer_error(void)
{
    calcinfo calc; /* calc情報構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(error_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, error_data[i].expr);

        TEST_ASSERT_STR_MSG(error_data[i].answer, (char *)calc.answer, "%s=%s",
                                            error_data[i].expr,
                                            error_data[i].answer);
        destroy_answer(&calc);
    }
    PASS();
}

/**
 * parse_func_args() 関数テスト
 *
 * @return なし
 */
TEST
test_parse_func_args(void)
{
    double x = 0.0, y = 0.0; /* 値 */
    calcinfo calc;           /* calc情報構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "(235)");
    st_calc.readch(&calc);

    dbglog("ch=%c", calc.ch);
    parse_func_args(&calc, &x, NULL);
    TEST_ASSERT_DOUBLE(235, 0.0, x);

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "(123,235)");
    st_calc.readch(&calc);

    dbglog("ch=%c", calc.ch);
    parse_func_args(&calc, &x, &y, NULL);
    TEST_ASSERT_DOUBLE(123, 0.0, x);
    TEST_ASSERT_DOUBLE(235, 0.0, y);
    PASS();
}

/**
 * set_digit() 関数テスト
 *
 * @return なし
 */
TEST
test_set_digit(void)
{
    calcinfo calc;                            /* calc情報構造体 */
    const char *expr = "sin(2)";              /* 式 */
    const char *expect = "0.909297426825682"; /* 期待する文字列 */

    set_digit(15L);
    (void)memset(&calc, 0, sizeof(calcinfo));
    exec_calc(&calc, expr);
    TEST_ASSERT_STR_MSG(expect, (char *)calc.answer, "%s=%s",
                                        expect, calc.answer);
    PASS();
}

/**
 * readch() 関数テスト
 *
 * @return なし
 */
TEST
test_readch(void)
{
    unsigned char *ptr = NULL; /* ポインタ */
    calcinfo calc;             /* calc情報構造体 */

    /* スペース・タブを含んだ文字列を設定 */
    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "te s  t");
    st_calc.readch(&calc);

    dbglog("ch=%c", calc.ch);
    TEST_ASSERT_INT_MSG('t', calc.ch, "%c=%c", 't', calc.ch);
    st_calc.readch(&calc);
    TEST_ASSERT_INT_MSG('e', calc.ch, "%c=%c", 'e', calc.ch);
    st_calc.readch(&calc);
    TEST_ASSERT_INT_MSG('s', calc.ch, "%c=%c", 's', calc.ch);
    st_calc.readch(&calc);
    TEST_ASSERT_INT_MSG('t', calc.ch, "%c=%c", 't', calc.ch);
    st_calc.readch(&calc);
    TEST_ASSERT_INT_MSG('\0', calc.ch, "ptr=%p", calc.ptr);
    ptr = calc.ptr; /* アドレス保持 */
    st_calc.readch(&calc);
    TEST_ASSERT_INT_MSG('\0', calc.ch, "ptr=%p", calc.ptr);
    /* アドレスが変わらないことを確認 */
    ASSERT_EQ(ptr, calc.ptr);
    PASS();
}

/**
 * expression() 関数テスト
 *
 * @return なし
 */
TEST
test_expression(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(expression_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, expression_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.expression(&calc);
        TEST_ASSERT_DOUBLE_MSG(expression_data[i].answer, 0.0, result, "%s=%.12g",
                                            expression_data[i].expr,
                                            expression_data[i].answer);
    }
    PASS();
}

/**
 * term() 関数テスト
 *
 * @return なし
 */
TEST
test_term(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(term_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, term_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.term(&calc);
        TEST_ASSERT_DOUBLE_MSG(term_data[i].answer, 0.0, result, "%s=%.12g",
                                            term_data[i].expr,
                                            term_data[i].answer);
        TEST_ASSERT_INT_MSG((int)term_data[i].errorcode, (int)calc.errorcode, "%s error",
                                         term_data[i].expr);
        clear_error(&calc);
    }
    PASS();
}

/**
 * factor() 関数テスト
 *
 * @return なし
 */
TEST
test_factor(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(factor_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, factor_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.factor(&calc);
        TEST_ASSERT_DOUBLE_MSG(factor_data[i].answer, 0.0, result, "%s=%.12g",
                                            factor_data[i].expr,
                                            factor_data[i].answer);
        TEST_ASSERT_INT_MSG((int)factor_data[i].errorcode, (int)calc.errorcode, "%s error",
                                         factor_data[i].expr);
        clear_error(&calc);
    }
    PASS();
}

/**
 * factor() 関数テスト
 *
 * @return なし
 */
TEST
test_token(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(token_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, token_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.token(&calc);
        TEST_ASSERT_DOUBLE_MSG(token_data[i].answer, 0.0, result, "%s=%.12g",
                                            token_data[i].expr,
                                            token_data[i].answer);
        TEST_ASSERT_INT_MSG((int)token_data[i].errorcode, (int)calc.errorcode, "%s error",
                                         token_data[i].expr);
        clear_error(&calc);
    }
    PASS();
}

/**
 * number() 関数テスト
 *
 * @return なし
 */
TEST
test_number(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(number_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, number_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.number(&calc);
        TEST_ASSERT_DOUBLE_MSG(number_data[i].answer, 0.0, result, "%s=%.12g",
                                            number_data[i].expr,
                                            number_data[i].answer);
    }
    PASS();
}

/**
 * get_strlen() 関数テスト
 *
 * @return なし
 */
TEST
test_get_strlen(void)
{
    TEST_ASSERT_INT(5, st_calc.get_strlen(50000, "%.18g"));
    TEST_ASSERT_INT(15, st_calc.get_strlen(123456789012345LL, "%.18g"));
    // 12345678.9000000004
    TEST_ASSERT_INT(19, st_calc.get_strlen(12345678.9, "%.18g"));
    // 1234567.89012344996
    TEST_ASSERT_INT(19, st_calc.get_strlen(1234567.89012345, "%.18g"));

    TEST_ASSERT_INT(5, st_calc.get_strlen(50000, "%.15g"));
    TEST_ASSERT_INT(15, st_calc.get_strlen(123456789012345LL, "%.15g"));
    // 12345678.9
    TEST_ASSERT_INT(10, st_calc.get_strlen(12345678.9, "%.15g"));
    // 1234567.89012345
    TEST_ASSERT_INT(16, st_calc.get_strlen(1234567.89012345, "%.15g"));

    // 5e+04
    TEST_ASSERT_INT(5, st_calc.get_strlen(50000, "%.1g"));
    // 1e+14
    TEST_ASSERT_INT(5, st_calc.get_strlen(123456789012345LL, "%.1g"));
    // 1e+07
    TEST_ASSERT_INT(5, st_calc.get_strlen(12345678.9, "%.1g"));
    // 1e+06
    TEST_ASSERT_INT(5, st_calc.get_strlen(1234567.89012345, "%.1g"));

    TEST_ASSERT_INT(5, st_calc.get_strlen(50000, "%.30g"));
    TEST_ASSERT_INT(15, st_calc.get_strlen(123456789012345LL, "%.30g"));
    // 12345678.9000000003725290298462
    TEST_ASSERT_INT(31, st_calc.get_strlen(12345678.9, "%.30g"));
    // 1234567.89012344996444880962372
    TEST_ASSERT_INT(31, st_calc.get_strlen(1234567.89012345, "%.30g"));
    PASS();
}

/**
 * 計算実行
 *
 * @param[in] calc calcinfo構造体
 * @param[in] str 文字列
 * @return calcinfo構造体
 */
static void
exec_calc(calcinfo *calc, const char *str)
{
    if (!create_answer(calc, (unsigned char *)str)) {
        TEST_ERROR("create_answer: calc=%p", (void *)calc);
        exit(EXIT_FAILURE);
    }
    dbglog("%p answer=%s", calc->answer, calc->answer);
}


/**
 * 初期化処理
 *
 * @return なし
 */
static void
setup(void *data)
{
    (void)data;
    TEST_PASSTHROUGH_RESET(strdup);
    fail_snprintf = SNPRINTF_NONE;
    fail_malloc_count = 0;
    FFF_RESET_HISTORY();
}

/**
 * 終了処理
 *
 * @return なし
 */
static void
teardown(void *data)
{
    (void)data;
    free_strings();
}

/**
 * create_answer() 関数テスト (処理時間の表示)
 *
 * @return なし
 */
TEST
test_answer_timer(void)
{
    calcinfo calc; /* calcinfo構造体 */

    g_tflag = true;
    (void)memset(&calc, 0, sizeof(calcinfo));
    exec_calc(&calc, "(105+312)+2*(5-3)");
    g_tflag = false;

    TEST_ASSERT_STR("421", (char *)calc.answer);
    destroy_answer(&calc);
    PASS();
}

/**
 * create_answer() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_answer_failure(void)
{
    calcinfo calc;                       /* calcinfo構造体 */
    unsigned char expr[] = "1+2";        /* 式 (答えは "3") */
    unsigned char third[] = "1/3";       /* 式 (答えは "0.333333333333") */
    unsigned char zero[] = "1/0";        /* 式 (0 で割る) */

    /* 書式の設定に失敗 */
    (void)memset(&calc, 0, sizeof(calcinfo));
    fail_snprintf = SNPRINTF_FORMAT;
    TEST_ASSERT_NULL(create_answer(&calc, expr));
    TEST_ASSERT_INT(SNPRINTF_NONE, fail_snprintf);

    /* 文字数の取得に失敗 (get_strlen) */
    (void)memset(&calc, 0, sizeof(calcinfo));
    fail_snprintf = SNPRINTF_STRLEN;
    TEST_ASSERT_NULL(create_answer(&calc, expr));
    TEST_ASSERT_INT(SNPRINTF_NONE, fail_snprintf);

    /* 値の文字列への変換に失敗 */
    (void)memset(&calc, 0, sizeof(calcinfo));
    fail_snprintf = SNPRINTF_ANSWER;
    TEST_ASSERT_NULL(create_answer(&calc, expr));
    TEST_ASSERT_INT(SNPRINTF_NONE, fail_snprintf);
    destroy_answer(&calc);

    /* メモリを確保できない ("0.333333333333" の 14 文字 + 終端で 15 バイト.
     * デバッグビルドの dbglog() などが確保するサイズと, 重ならない長さにする) */
    (void)memset(&calc, 0, sizeof(calcinfo));
    fail_malloc_size = 15;
    fail_malloc_count = 1;
    TEST_ASSERT_NULL(create_answer(&calc, third));
    TEST_ASSERT_INT(0, fail_malloc_count);

    /* エラーメッセージを作れない */
    (void)memset(&calc, 0, sizeof(calcinfo));
    TEST_INJECT(strdup, 0, 1, NULL, ENOMEM);
    TEST_ASSERT_NULL(create_answer(&calc, zero));
    TEST_ASSERT_INJECTED(strdup);
    PASS();
}

/**
 * 式を解析する関数テスト (エラー状態のときは, 計算せずに EX_ERROR を返す)
 *
 * @return なし
 */
TEST
test_calc_error_state(void)
{
    calcinfo calc;               /* calcinfo構造体 */
    const double EX_ERROR = 0.0; /* エラー戻り値 (calc.c の内部定数と同じ) */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "1+2");
    st_calc.readch(&calc);
    set_errorcode(&calc, E_SYNTAX);

    TEST_ASSERT_DOUBLE(EX_ERROR, 0.0, st_calc.expression(&calc));
    TEST_ASSERT_DOUBLE(EX_ERROR, 0.0, st_calc.term(&calc));
    TEST_ASSERT_DOUBLE(EX_ERROR, 0.0, st_calc.factor(&calc));
    TEST_ASSERT_DOUBLE(EX_ERROR, 0.0, st_calc.token(&calc));
    PASS();
}

/**
 * parse_func_args() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_parse_func_args_failure(void)
{
    calcinfo calc; /* calcinfo構造体 */
    double x = 0.0; /* 値 */

    /* 引数の始まりが '(' ではない */
    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "abc");
    st_calc.readch(&calc);
    parse_func_args(&calc, &x, NULL);
    ASSERT(is_error(&calc));
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    startup();
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(test_answer_four);
    RUN_TEST(test_answer_timer);
    RUN_TEST(test_answer_failure);
    RUN_TEST(test_calc_error_state);
    RUN_TEST(test_parse_func_args_failure);
    RUN_TEST(test_answer_func);
    RUN_TEST(test_answer_four_func);
    RUN_TEST(test_answer_error);
    RUN_TEST(test_parse_func_args);
    RUN_TEST(test_set_digit);
    RUN_TEST(test_readch);
    RUN_TEST(test_expression);
    RUN_TEST(test_term);
    RUN_TEST(test_factor);
    RUN_TEST(test_token);
    RUN_TEST(test_number);
    RUN_TEST(test_get_strlen);
    TEST_MAIN_END();
}

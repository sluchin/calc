/**
 * @file  tests/lib/test_sig.c
 * @brief lib/sig.c の単体テスト
 *
 * @author higashi
 * @date 2026-10-04 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2026 Tetsuya Higashi. All Rights Reserved.
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

#include <signal.h> /* sigaction sigfillset sigismember */
#include <string.h> /* memset */
#include <errno.h>  /* errno EINVAL */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "sig.h"

DEFINE_FFF_GLOBALS

/* システムコールは, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(int, sigaction, int, const struct sigaction *, struct sigaction *)
TEST_PASSTHROUGH(int,
                 sigaction,
                 (int signo, const struct sigaction *act, struct sigaction *oldact),
                 (signo, act, oldact))
FAKE_VALUE_FUNC(int, sigfillset, sigset_t *)
TEST_PASSTHROUGH(int, sigfillset, (sigset_t * set), (set))

/* プロトタイプ */
TEST test_set_sigaction(void);
TEST test_set_sigaction_ignore_and_flags(void);
TEST test_set_sigaction_failure(void);

/* greatest の定義 (main() を含む, 実行ファイルごとに 1 か所) */
GREATEST_MAIN_DEFS();

/** テストに使うシグナル (既定では, プロセスを終了させる. 送らないので, 影響はない) */
static const int test_signo = SIGUSR1;
/** テスト前のシグナルの設定 (終了処理で戻す) */
static struct sigaction saved;

/**
 * テスト用のハンドラ (呼ばれない. 設定されたことを, アドレスで確認する)
 *
 * @param[in] signo シグナル番号
 */
static void
test_handler(int signo)
{
    (void)signo; /* 使用しない */
}

/**
 * 現在の設定を取得する
 *
 * @param[out] sa 取得した設定
 * @retval 0 正常
 * @retval -1 エラー
 */
static int
get_action(struct sigaction *sa)
{
    return sigaction(test_signo, NULL, sa);
}

/**
 * 初期化処理 (テスト前のシグナルの設定を保存する)
 *
 * @param[in] data 使用しない
 */
static void
setup(void *data)
{
    (void)data; /* 使用しない */
    TEST_PASSTHROUGH_RESET(sigaction);
    TEST_PASSTHROUGH_RESET(sigfillset);
    FFF_RESET_HISTORY();
    if (get_action(&saved) < 0)
        TEST_NOTIFY("sigaction: get(%d)", errno);
}

/**
 * 終了処理 (シグナルの設定を戻す)
 *
 * @param[in] data 使用しない
 */
static void
teardown(void *data)
{
    (void)data; /* 使用しない */
    TEST_PASSTHROUGH_RESET(sigaction);
    if (sigaction(test_signo, &saved, NULL) < 0)
        TEST_NOTIFY("sigaction: restore(%d)", errno);
}

/**
 * テストの実行
 *
 * @param[in] argc 引数の数
 * @param[in] argv 引数 (greatest のオプション. -t の後にテスト名を指定すると, 1 つのテストだけ実行できる)
 * @return 全てのテストが成功なら EXIT_SUCCESS, 失敗があれば EXIT_FAILURE
 */
int
main(int argc, char **argv)
{
    /* greatest の初期化 (オプションの解析. 標準出力のバッファリングは行わない) */
    TEST_MAIN_BEGIN();
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    /* テストの実行 */
    RUN_TEST(test_set_sigaction);
    RUN_TEST(test_set_sigaction_ignore_and_flags);
    RUN_TEST(test_set_sigaction_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}

/**
 * set_sigaction() 関数テスト
 */
TEST
test_set_sigaction(void)
{
    struct sigaction sa; /* 設定後の sigaction構造体 */
    int retval = 0;      /* 戻り値 */
    int member = 0;      /* sigismember戻り値 */

    (void)memset(&sa, 0, sizeof(struct sigaction));

    /* ハンドラを設定する */
    TEST_ASSERT_INT(EX_OK, set_sigaction(test_signo, test_handler, 0));
    retval = get_action(&sa);
    TEST_ASSERT_INT(0, retval);
    TEST_ASSERT_MSG(sa.sa_handler == test_handler, "sa.sa_handler == test_handler");

    /* ハンドラの実行中は, 全てのシグナルを受け付けない */
    member = sigismember(&sa.sa_mask, SIGUSR2);
    TEST_ASSERT_INT(1, member);
    member = sigismember(&sa.sa_mask, SIGINT);
    TEST_ASSERT_INT(1, member);
    PASS();
}

/**
 * set_sigaction() 関数テスト (無視, フラグの追加)
 */
TEST
test_set_sigaction_ignore_and_flags(void)
{
    struct sigaction sa; /* 設定後の sigaction構造体 */
    int retval = 0;      /* 戻り値 */

    (void)memset(&sa, 0, sizeof(struct sigaction));

    /* 無視する (フラグを追加する) */
    TEST_ASSERT_INT(EX_OK, set_sigaction(test_signo, SIG_IGN, SA_NODEFER));
    retval = get_action(&sa);
    TEST_ASSERT_INT(0, retval);
    TEST_ASSERT_MSG(sa.sa_handler == SIG_IGN, "sa.sa_handler == SIG_IGN");
    TEST_ASSERT_MSG((sa.sa_flags & SA_NODEFER) != 0, "(sa.sa_flags & SA_NODEFER) != 0");

    /* それまでのフラグは, 消えない (フラグは, 追加される) */
    TEST_ASSERT_INT(EX_OK, set_sigaction(test_signo, test_handler, SA_RESTART));
    retval = get_action(&sa);
    TEST_ASSERT_INT(0, retval);
    TEST_ASSERT_MSG(sa.sa_handler == test_handler, "sa.sa_handler == test_handler");
    TEST_ASSERT_MSG((sa.sa_flags & SA_NODEFER) != 0, "(sa.sa_flags & SA_NODEFER) != 0");
    TEST_ASSERT_MSG((sa.sa_flags & SA_RESTART) != 0, "(sa.sa_flags & SA_RESTART) != 0");
    PASS();
}

/**
 * set_sigaction() 関数テスト (失敗)
 */
TEST
test_set_sigaction_failure(void)
{
    struct sigaction before; /* 失敗する前の sigaction構造体 */
    struct sigaction after;  /* 失敗したあとの sigaction構造体 */
    int retval = 0;          /* 戻り値 */
    unsigned int calls = 0u; /* 呼び出し回数 */

    retval = get_action(&before);
    TEST_ASSERT_INT(0, retval);

    /* 不正なシグナル番号 (取得に失敗する) */
    TEST_ASSERT_INT(EX_NG, set_sigaction(-1, test_handler, 0));

    /* 変更できないシグナル (SIGKILL: 取得はできて, 設定に失敗する) */
    TEST_ASSERT_INT(EX_NG, set_sigaction(SIGKILL, test_handler, 0));

    /* sigfillset() に失敗: sigaction() は呼ばれず, 設定は変わらない */
    calls = sigaction_fake.call_count;
    TEST_INJECT(sigfillset, 0, 1, -1, EINVAL);
    TEST_ASSERT_INT(EX_NG, set_sigaction(test_signo, test_handler, 0));
    TEST_ASSERT_INJECTED(sigfillset);
    TEST_ASSERT_INT(calls, sigaction_fake.call_count);

    /* 取得 (1 回目の sigaction()) に失敗 */
    TEST_INJECT(sigaction, 0, 1, -1, EINVAL);
    TEST_ASSERT_INT(EX_NG, set_sigaction(test_signo, test_handler, 0));
    TEST_ASSERT_INJECTED(sigaction);

    /* 設定 (2 回目の sigaction()) に失敗 */
    TEST_INJECT(sigaction, 1, 1, -1, EINVAL);
    TEST_ASSERT_INT(EX_NG, set_sigaction(test_signo, test_handler, 0));
    TEST_ASSERT_INJECTED(sigaction);

    /* どの失敗でも, 設定は変わらない */
    TEST_PASSTHROUGH_RESET(sigaction);
    retval = get_action(&after);
    TEST_ASSERT_INT(0, retval);
    TEST_ASSERT_MSG(after.sa_handler == before.sa_handler, "after.sa_handler == before.sa_handler");
    PASS();
}

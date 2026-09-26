/**
 * @file tests/test_process.h
 * @brief 単体テスト共通ヘッダ (子プロセスでの実行)
 *
 * exit() を呼ぶ関数 (parse_args() や main()) を, 子プロセスで実行して,
 * 終了ステータスと出力を確認するための関数群.
 * 子プロセスは exit() で終了する (_exit() だとカバレッジが出力されない).
 */

#ifndef _TEST_PROCESS_H_
#define _TEST_PROCESS_H_

#include <stdio.h>    /* fflush */
#include <stdlib.h>   /* exit EXIT_SUCCESS */
#include <string.h>   /* strlen */
#include <unistd.h>   /* fork pipe dup2 read write close */
#include <sys/mman.h> /* mmap munmap */
#include <sys/wait.h> /* waitpid */
#include <pty.h>      /* forkpty */

/** 子プロセスで実行する関数 */
typedef void (*test_child_func)(void *arg);

/**
 * 子プロセスで関数を実行する
 * 標準入力に input を渡し, 標準出力と標準エラー出力を out に取得する.
 * 関数から戻った場合は, exit(EXIT_SUCCESS) で終了する.
 *
 * @param[in] func 実行する関数
 * @param[in] arg 関数の引数
 * @param[in] input 標準入力に渡す文字列 (NULL可, パイプに収まる長さ)
 * @param[out] out 出力の取得先 (NULL可)
 * @param[in] outsize out のサイズ
 * @return 終了ステータス, シグナルで終了した場合は 128 + シグナル番号
 * @retval -1 プロセス生成などに失敗
 */
static inline int
test_run_child(test_child_func func, void *arg, const char *input,
               char *out, size_t outsize)
{
    int inpipe[2] = { -1, -1 };  /* 標準入力用 */
    int outpipe[2] = { -1, -1 }; /* 出力用 */
    pid_t cpid = 0;              /* 子プロセスID */
    int status = 0;              /* ステータス */
    size_t total = 0;            /* 取得したバイト数 */
    ssize_t len = 0;             /* read戻り値 */

    if (pipe(inpipe) < 0 || pipe(outpipe) < 0)
        return -1;

    if (input && write(inpipe[1], input, strlen(input)) < 0)
        return -1;
    (void)close(inpipe[1]);

    (void)fflush(NULL);
    cpid = fork();
    if (cpid < 0)
        return -1;

    if (cpid == 0) { /* 子プロセス */
        (void)close(outpipe[0]);
        if (dup2(inpipe[0], STDIN_FILENO) < 0 ||
            dup2(outpipe[1], STDOUT_FILENO) < 0 ||
            dup2(outpipe[1], STDERR_FILENO) < 0)
            exit(EXIT_FAILURE);
        (void)close(inpipe[0]);
        (void)close(outpipe[1]);
        func(arg);
        exit(EXIT_SUCCESS);
    }

    /* 親プロセス */
    (void)close(inpipe[0]);
    (void)close(outpipe[1]);
    if (out && outsize > 0) {
        out[0] = '\0';
        while (total < outsize - 1 &&
               (len = read(outpipe[0], out + total, outsize - 1 - total)) > 0)
            total += (size_t)len;
        out[total] = '\0';
    }
    /* 残りは読み捨てる (子プロセスが SIGPIPE で終了しないように) */
    {
        char dummy[256];
        while (read(outpipe[0], dummy, sizeof(dummy)) > 0)
            ;
    }
    (void)close(outpipe[0]);

    if (waitpid(cpid, &status, 0) < 0)
        return -1;
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return -1;
}

/**
 * 疑似端末 (pty) つきの子プロセスで関数を実行する
 * 子プロセスの標準入出力が端末になるので, readline などの端末用の処理を確認できる.
 * input を端末に入力し, 端末の出力 (入力のエコーも含む) を out に取得する.
 *
 * @param[in] func 実行する関数
 * @param[in] arg 関数の引数
 * @param[in] input 端末に入力する文字列 (NULL可)
 * @param[out] out 出力の取得先 (NULL可)
 * @param[in] outsize out のサイズ
 * @return 終了ステータス, シグナルで終了した場合は 128 + シグナル番号
 * @retval -1 プロセス生成などに失敗
 */
static inline int
test_run_child_pty(test_child_func func, void *arg, const char *input,
                   char *out, size_t outsize)
{
    int master = -1;  /* 端末のマスタ側 */
    pid_t cpid = 0;   /* 子プロセスID */
    int status = 0;   /* ステータス */
    size_t total = 0; /* 取得したバイト数 */
    ssize_t len = 0;  /* read戻り値 */
    char dummy[256];  /* 読み捨て用 */

    (void)fflush(NULL);
    cpid = forkpty(&master, NULL, NULL, NULL);
    if (cpid < 0)
        return -1;

    if (cpid == 0) { /* 子プロセス */
        (void)setenv("TERM", "dumb", 1); /* 制御文字を減らす */
        func(arg);
        exit(EXIT_SUCCESS);
    }

    /* 親プロセス */
    if (input && write(master, input, strlen(input)) < 0) {
        (void)close(master);
        return -1;
    }
    if (out && outsize > 0)
        out[0] = '\0';
    /* 子プロセスが終了して, 端末を閉じるまで読む (Linux では, EIO が返る) */
    while ((len = read(master, dummy, sizeof(dummy))) > 0) {
        if (out && total < outsize - 1) {
            size_t n = ((size_t)len < outsize - 1 - total) ?
                (size_t)len : outsize - 1 - total;
            (void)memcpy(out + total, dummy, n);
            total += n;
            out[total] = '\0';
        }
    }
    (void)close(master);

    if (waitpid(cpid, &status, 0) < 0)
        return -1;
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return -1;
}

/**
 * 親子プロセスで共有するメモリの確保
 * モックの呼び出し回数などを, 子プロセスから親プロセスに伝えるために使う.
 * 内容は 0 で初期化される.
 *
 * @param[in] size バイト数
 * @return 共有メモリ
 * @retval NULL 失敗
 */
static inline void *
test_shared_alloc(size_t size)
{
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? NULL : ptr;
}

/**
 * 共有メモリの解放
 *
 * @param[in] ptr 共有メモリ
 * @param[in] size バイト数
 * @return なし
 */
static inline void
test_shared_free(void *ptr, size_t size)
{
    if (ptr)
        (void)munmap(ptr, size);
}

#endif /* _TEST_PROCESS_H_ */

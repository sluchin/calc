# プロジェクト設定

C 言語 (gcc / GNU make) の電卓プログラム。スタンドアロン版 (`calcp`) と、クライアントサーバ版 (`calcd` / `calcc`) がある。

## コマンド

- ビルド: `make clean all` (リリース) または `make clean debug` (デバッグ。`-D_DEBUG -DUNITTEST` を付け、`dbglog` などのデバッグログを有効にする)
  - 最上位の `GNUmakefile` が `lib` → `calc` → `server` → `client` の順に各ディレクトリの `Makefile` を呼ぶ。`lib` を先にビルドしないと、他が `libcalcutil` をリンクできない。
  - ライブラリは、静的 (`.a`) と動的 (`.so`) の、どちらか一方だけを作る。既定は静的で、`make DYNAMIC=1` (CMake は `-DDYNAMIC=ON`) で動的。切り替えるときは `make clean` する。GNU make の `-D` は、変数の定義ではないので、`DYNAMIC=1` と書く。
  - `make strip`: strip。`make install`: `/usr/local` へインストールして、ライブラリの種類に応じたメッセージ (動的のときは `ldconfig` の案内) を表示する。
  - 単体テストは、ライブラリの種類に関係なく、テスト専用の共有ライブラリ (`*_testlib`、インストールしない) にリンクする。モック (同名の関数の定義) が、静的ライブラリでは多重定義になるため。
  - GNU readline (`libreadline-dev`) が必要。
- CMake でのビルド: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build` (`Release` も可。`make cmake-build` でも実行できる)
  - 最上位の `CMakeLists.txt` 1 つで全体を扱う。Debug は `-DUNITTEST -D_DEBUG`、それ以外は `-DNDEBUG` を付ける。
  - 最適化は `Makefile` に合わせて `-g -O2` にしている。
  - 単体テストは、`make` (引数なし) ではビルドされない。`build/` で `make test` (`ctest`) を実行すると、ビルドしてから実行する (ビルドは、ctest のテスト `build_tests`。全てのテストが `FIXTURES_REQUIRED` で依存する)。`make test` は、ビルドの出力と各テストの出力を全て表示する (`CMAKE_CTEST_ARGUMENTS` に `--verbose`。CMake 3.17 以降)。`BUILD_TESTS` は既定で ON で、ON のときは全体に `-DUNITTEST` を付ける。配布用のビルドは `-DBUILD_TESTS=OFF`。
  - 生成物は `build/` に出る (`.gitignore` 済み)。ソースファイルを追加したら、`CMakeLists.txt` も更新する。
- 警告: ビルド出力に警告を出さないこと。警告が出たら修正する。
  - 警告オプションの一覧は、`warnings.txt` にある (`-Wall` `-Wextra` `-Wpedantic` `-Wconversion` `-Wsign-conversion` `-Wshadow` `-Wformat=2` `-Wcast-qual` など)。Makefile (`warnings.mk`) と CMake (`cmake/warnings.cmake`) の両方が、この一覧を読む。オプションを足したり外したりするときは、`warnings.txt` だけを直す。
  - コンパイラが受け付けないオプションは、自動で外れる (古い gcc でもビルドできる)。
  - 使用しているライブラリのヘッダが警告を出すときは、そのヘッダの `#include` を、次のように囲む (`-W` のオプションを外すのではなく)。警告が出ないヘッダは、囲まない。
    ```c
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #include "xxx"
    #pragma GCC diagnostic pop
    ```
  - ログ関数 (`outlog` `dbglog` `outstd` `outdump` など) は、`printf` 形式の属性 (`LOG_FORMAT`) を持つので、書式と引数の型が合わないと、警告になる。`%p` には `(const void *)` にキャストした引数を渡す。`%x` に構造体を渡さない。
  - 警告を `#pragma GCC diagnostic ignored` で抑えるのは、理由が明らかなときだけ (例: 桁数から作る書式文字列の `-Wformat-nonliteral`) にして、理由をコメントに書く。
  - 単体テスト (`tests/CMakeLists.txt`) だけ、次の 2 つを外している。`-Wwrite-strings` (テストの `argv` を文字列リテラルで初期化するため)、`-Wredundant-decls` (モックが libc の関数を再宣言するため)。
- テスト: `make test` (CMake の Debug ビルドを `build-test/` に作り、ビルドして `ctest` を実行する。CMake が必要)
  - 1 つだけ実行するときは、`build-test/tests/<ディレクトリ>/<テスト名>` を直接実行する (先に `make test` でビルドしておく) (例: `build-test/tests/lib/test_data`)。greatest のオプション (`-t <名前>` で名前を指定、`-l` で一覧) が使える。
  - テストが実行できなかったときは、そのことを報告する (成功したとは書かない)。
- カバレッジ: `make coverage` (gcov と gcovr が必要。`build-coverage/` に作り、端末に一覧を出す。詳細は `build-coverage/coverage/index.html`)
  - CMake のビルドディレクトリでも `make coverage` が使える。`-DENABLE_COVERAGE=ON` (最適化なしで `--coverage` を付ける) で構成していなければ、`<ビルドディレクトリ>/coverage-build/` に別にビルドする。`gcovr` が無いと、理由を表示して失敗する。
  - `add_custom_target` には、必ず `VERBATIM` を付ける (付けないと、`(` などを含む引数でシェルの構文エラーになる)。
- 静的解析: `make analyze` (gcc 10 以降の `-fanalyzer`。通常のビルドとは分けてあり、リリース用とデバッグ用の両方の設定で、全ソースを `-O0` で解析する。指摘があれば失敗する)。CMake では `cmake --build <ビルドディレクトリ> --target analyze`。コードを修正したら、実行して、指摘が出ないことを確認する。
  - 指摘が、意図した書き方による誤検知のとき (例: `dup2` で、直前に閉じた番号に複製する) だけ、`#pragma GCC diagnostic ignored "-Wanalyzer-..."` で抑える。理由をコメントに書き、gcc 10 未満では `#if` で無効にする。
- ドキュメント: `build/` で `make doc` (doxygen が必要。`build/docs/html/index.html` に出力。`make clean` で削除される)。`make clean` は、`docs` `coverage` `coverage-build` `Testing` と、`tests/` の実行ファイルのディレクトリを、ディレクトリごと削除する (CMake 3.15 以降。`tests/` 自体は、ビルドに必要な生成ファイルがあるので残す。実行ファイルのディレクトリは、リンカが作らないので、リンクの前に作る)。設定は `Doxyfile` (標準と異なる項目だけを書く。`cmake/doxygen.cmake` が、出力先と `HAVE_DOT` を追加して上書きする)

- CI: `.github/workflows/ci.yml` (GitHub Actions)。`build` (リリースとデバッグ。警告があれば失敗)、`test`、`analyze`、`coverage` (`CMakeLists.txt` の `COVERAGE_MIN_LINE` と `COVERAGE_MIN_BRANCH` を下回ると失敗)、`docs` (警告があれば失敗)、`format` (整形が必要なら失敗) の各ジョブ。ローカルでも、同じコマンド (`make analyze` `make coverage` `make format-check` など) で確認できる。

## プロジェクト構成

- `lib/`: 共通ライブラリ `libcalcutil` (ログ `log`、ネットワーク `net`、送受信データ `data`、ファイル `fileio`、端末 `term`、シグナル設定 `sig`、`readline`、`memfree`、`timer.h`)。他のディレクトリから使われる。`lib` は他に依存しない。
- `calc/`: 計算処理 `libcalcp` (`calc.c`、`func.c`、`error.c`) と、スタンドアロン版の `calcp` (`main.c`、`option.c`)。
- `server/`: サーバデーモン `calcd`。
- `client/`: クライアント `calcc`。
- `tests/`: 単体テスト。ソースと同じ構成で、`tests/lib/` (`lib`)、`tests/calcp/` (`calc`)、`tests/calcd/` (`server`)、`tests/calcc/` (`client`) に置く。`tests/third_party/` に、テストランナー `greatest.h` とモック `fff.h` (どちらもヘッダのみ) がある。
- `def.h` の `EX_OK` / `EX_NG` を、関数の戻り値に使う。
- 生成物 (`*.o` `*.a` `*.so`、`calcp` `calcc` `calcd`) は `.gitignore` 済み。コミットしない。

## コーディング規約

@../CODING_RULES.md

## テストの規約

- テストランナーは greatest、モックは FFF (`fff.h`)。共通のマクロは `tests/test_helper.h` にある (`TEST_ASSERT_INT` `TEST_ASSERT_STR` `TEST_ASSERT_MATCH` `TEST_FAIL` `TEST_NOTIFY` など)。
- テストは `tests/<ディレクトリ>/test_<ソース名>.c` に置く (`lib/data.c` → `tests/lib/test_data.c`)。1 ファイルが、1 つの実行ファイルと、1 つの ctest テストになる。
- 関数名は `test_<テスト対象の関数名>` (`set_client_data` → `test_set_client_data`)。テストは `TEST` で定義して `PASS()` で終わり、`main` の `RUN_TEST` に登録する。初期化・後始末は、`startup()` / `SET_SETUP` / `SET_TEARDOWN` で行う。
- `option.c` と `main.c` のテスト (`test_option.c` `test_main.c`) は、ソースごと実行ファイルにリンクし、呼び出す関数を FFF でモックにする。`main()` は `RENAME_MAIN` で名前を変える (`tests/CMakeLists.txt`)。`exit()` を呼ぶので、`tests/test_process.h` の `test_run_child()` で子プロセスで実行し、終了ステータスと出力を確認する。子プロセスのモックの呼び出し回数は、親から見えないので、`test_shared_alloc()` の共有メモリに、`custom_fake` で記録する。
- `TEST_FAIL` `TEST_ASSERT_*` は、`TEST` 関数の中でしか使えない (失敗すると `return` するため)。`TEST` 以外の補助関数や、`fork` した子プロセスでは、`TEST_ERROR` で表示し、戻り値や `exit()` で伝える。
- メッセージは標準出力に出す (テストが標準エラー出力をパイプに繋ぐため)。
- 外部の関数 (システムコールなど) を置き換えるときは、FFF の `FAKE_VALUE_FUNC` / `FAKE_VOID_FUNC` を使う (例: `tests/lib/test_term.c` の `tcgetattr`)。`DEFINE_FFF_GLOBALS` は、1 つの実行ファイルに 1 か所だけ書く。`SET_SETUP` で `RESET_FAKE` を呼び、テスト間でモックの状態を持ち越さない。
- libc の関数の失敗 (`send` `recv` `close` `socket` `malloc` など) を通すときは、`tests/test_helper.h` の `TEST_PASSTHROUGH` を使う。FFF のモックにして、通常は `dlsym(RTLD_NEXT)` で本物を呼び (素通し)、`TEST_INJECT(名前, 見送る回数, 失敗させる回数, 戻り値, errno)` で、その回数だけ失敗させる。`setup` で `TEST_PASSTHROUGH_RESET(名前)` を呼ぶ。`TEST_ASSERT_INJECTED(名前)` で、失敗が使われたこと (関数が呼ばれたこと) を確認する。実際に失敗させられる場合 (不正なファイルディスクリプタ、閉じた接続先、巨大な `malloc` など) は、モックにしない。
  - `snprintf` `fcntl` (可変引数) や `malloc` `calloc` のように、ほとんどの関数が使うものは、FFF のモックにしない (FFF のモックは、`main()` より前の呼び出しにも使われる)。テストのファイルに、同名の関数を直接定義して、指定した条件のときだけ失敗させる (`tests/calcp/test_calc.c` `tests/calcd/test_server.c` `tests/lib/test_net.c`)。本物は、`__libc_malloc` `dlsym(RTLD_NEXT)` `vsnprintf` で呼ぶ。`malloc` のあとに `memset(0)` するコードは、最適化で `calloc` になるので、`calloc` も置き換える。デバッグビルドの `dbglog()` も、`snprintf` や `malloc` を呼ぶので、呼び出しの回数ではなく、書式やサイズで、失敗させる呼び出しを選ぶ。
  - `atexit` は、glibc の静的ライブラリ (`libc_nonshared.a`) の小さな関数 (スタブ) で、共有ライブラリの中に取り込まれるので、直接は置き換えられない。スタブが呼ぶ `__cxa_atexit` (libc.so の関数) を置き換える (`tests/calcc/test_client.c`)。テストの実行ファイルにリンクした `main.c` の `atexit` は、直接置き換えられる (`tests/calcc/test_main.c`)。本物は `__cxa_atexit` で呼ぶ。
  - `exit()` や `fork` を伴う処理 (`main()` など) の失敗は、注入も、子プロセスの中で行う。親の `test_run_child()` が、`fflush` などを先に呼んで、注入を消費するため。
  - `_FORTIFY_SOURCE` が有効だと、`vsnprintf` などが `__vsnprintf_chk` に置き換わり、モックにできない。`BUILD_TESTS` が ON のときは、`-U_FORTIFY_SOURCE` を付けている (`CMakeLists.txt`)。
  - `pipe` (配列引数) や `va_list` を取る関数のモックは、`#pragma GCC diagnostic ignored` で警告を抑える (`tests/lib/test_fileio.c` `tests/lib/test_log.c`)。
  - 関数ポインタの引数は、FFF が直接書けないので、`typedef` する (`tests/calcd/test_server.c` の `thread_func_t`)。
  - 標準エラー出力が、前のテストで閉じたパイプのままだと、`SIGPIPE` で終了する。ログを出すテストの前に、`redirect(STDERR_FILENO, "/dev/null")` する。
- 端末や環境に依存させない。端末が無くても (`ctest` の標準入力は端末ではない) 実行できること。
- テストを追加したら、`tests/CMakeLists.txt` にも追加する。実行ファイルの名前は `<ディレクトリ>_<名前>` (例: `calcp_test_option`) で、`tests/<ディレクトリ>/<名前>` に出力される。
- `calcp` は、標準入力が端末のときだけ readline を使う。端末でない入力 (パイプやファイル) の `main()` は、`test_run_child()` (`tests/test_process.h`)、端末のとき (readline、履歴、イベントフック) は、疑似端末の `test_run_child_pty()` で確認する。
- `tests/third_party/` のファイルは、外部のソースなので、修正しない。

## 開発ルール

- コードの修正後は、必ず `make clean debug` (警告が出ないこと) と `make test` を実行し、エラーが出なくなるまで修正を繰り返す。
  - **例外**: 変更が `CLAUDE.md` や `README.md` `TODO.md` などの文書のみの場合は、ビルドとテストを実行しない。
- 機能追加・修正には、単体テストを追加する。
- オプションやコマンドを変更したら、`README.md` も更新する。
- ソースファイルを追加したら、該当ディレクトリの `Makefile` (`OBJECTS` / `OBJCALC` と、依存関係) と `CMakeLists.txt` も更新する。

### 完了条件・タスク完了時の動作

- 修正や機能追加が完了し、ビルドやテストが全て成功したら、以下の手順を実施してタスクを終了すること。
  1. `git diff --stat` を実行して、必要に応じて主要ファイルの差分を確認する。
  2. 修正内容のサマリー (変更点と理由の簡潔なまとめ) を、ユーザーに報告する。
- ユーザーに依頼されない限り、`git commit` / `git push` は行わない。

### コミットメッセージ

- 英語で書く。
- 本文 (タイトルの次の空行のあと) は、`-` で始まる箇条書きにする。
- `Co-Authored-By:` などの帰属行は含めない。

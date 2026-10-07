# MISRA-C 例外規定書 (MISRA-C Deviations)

本ドキュメントは、電卓プログラム (`calc` / `calcp` / `calcd` / `calcc`) における MISRA-C (MISRA C:2012 および MISRA C:2004) ガイドラインの適用方針と、プロジェクトの目的・アーキテクチャ上の理由から**あえて対応せず例外 (Deviation) としているルール**を定義・記録したものです。

---

## 1. 概要と適用方針

本ソフトウェアは、Linux/POSIX 環境で動作するコンソール対話型電卓 (`calcp`)、ネットワーク計算サーバデーモン (`calcd`)、および計算サーバイベント駆動クライアント (`calcc`) です。

組込み機器の静的ファームウェアとは異なり、汎用 OS 上での対話的入出力、ネットワーク通信、マルチスレッド並行処理、科学技術計算（浮動小数点演算）を主な目的としています。そのため、コードの安全性・堅牢性を高めるルールは積極的に採用しつつ、汎用 OS アプリケーションとしての設計・機能上不要または制約となるルールについては、安全対策（代替策）を講じた上で「例外 (Deviation)」として扱います。

### 準拠・対応している主なプラクティス
- **制御式の本質的ブール型評価 (MISRA C:2012 Rule 14.4)**: 条件式は bool だけにする。整数やポインタは、比較して bool にする (`if (ptr == NULL)`, `if (flag != 0)`, `if (strcmp(...) == 0)`)。bool の値 (`bool` 型の変数、`bool` を返す関数) は、そのまま書く (`if (g_tflag)`, `if (is_error(calc))`, `if (!exec)`。`== true` / `== false` とは比較しない)。
  - **許容している書き方**: 判定だけをする関数 (状態を調べるだけで、何も変えない関数。`is_error()`, `isdigit()`, `strcmp()`, `feof()`, `access()` など) は、戻り値を変数に入れずに、そのまま条件式に書く。厳密には、変数に入れてから判定するべきだが、読みやすさのために許容している (MISRA の Rule 13.5 は、`&&` / `||` の右側に、副作用のある関数呼び出しを書くことを禁じるもので、単独の判定は、対象外)。
  - **変数に入れる関数**: 副作用のある関数 (`sigaction()`, `close()`, `dup2()`, `setvbuf()`, `atexit()`, `gettimeofday()` など、状態を変える、書き込む、開く・閉じる、設定する関数) は、先に呼び出して、戻り値を変数に入れてから判定する (`retval = close(fd);` の次の行に `if (retval < 0)`)。
- **式中での代入排除 (MISRA C:2012 Rule 13.4)**: `while ((opt = getopt(...)) != EOF)` や `while ((val = va_arg(...)) != NULL)` などのループ条件式内の代入を分離。
- **型変換・リテラルの明示 (MISRA C:2012 Rule 10.x)**: 浮動小数点演算での `10.0` の明示など、暗黙の型変換を抑制。
- **符号なし整数リテラルの `U` サフィックス (MISRA C:2012 Rule 7.2)**: `size_t` などの符号なし型で使う整数リテラルに、大文字の `U` を付ける (`size_t len = 0U;`)。
- **`long` 系リテラルのサフィックス**: `long` / `ssize_t` には `L`、`long long` には `LL` (大文字。小文字の `l` は MISRA C:2012 Rule 7.3 により使わない)。
- **予約済み識別子の回避 (MISRA C:2012 Rule 21.1 / 21.2)**: アンダースコアで始まるヘッダインクルードガードを排除。
- **関数戻り値の確認 (MISRA C:2012 Rule 17.7)**: 戻り値を返す関数は必ず検証。あえて無視する場合は `(void)` キャストを明示（コンセプト 1, 2）。
- **安全なバッファ操作**: `strcpy`, `strcat`, `sprintf`, `gets` などの長さを見ない関数は禁止し、`snprintf` や境界チェックを徹底。
- **コンパイル警告ゼロ**: `warnings.txt` の警告オプション (`-Wall` `-Wextra` `-Wpedantic` `-Wconversion` `-Wsign-conversion` など約 50 個) で警告が出ない状態を維持。

---

## 2. 例外事項 (Deviation) サマリー一覧

| No. | MISRA C:2012 | MISRA C:2004 | 分類 | 概要 | プロジェクトでの主な該当箇所 |
|:---:|:---|:---|:---:|:---|:---|
| 1 | Rule 15.1 | Rule 14.4 | Advisory / Required | `goto` 文の使用 | エラーハンドラ・リソース一括解放 (`lib/net.c`, `server/server.c`) |
| 2 | Rule 21.3 | Rule 20.4 | Required | 動的ヒープメモリ確保・解放 | 可変長式・パケットバッファ (`calc/calc.c`, `lib/data.c`, `lib/memfree.c`) |
| 3 | Rule 17.2 | Rule 16.2 | Required | 関数の再帰呼び出し | 再帰下降構文解析 (`calc/calc.c`) |
| 4 | Rule 1.2 | Rule 1.1, 1.2 | Advisory / Required | POSIX 拡張・OS 固有 API の使用 | ソケット, スレッド, 端末制御, readline (`lib/`, `server/`, `client/`) |
| 5 | Rule 21.6 | Rule 20.9 | Required | 標準入出力ライブラリ `<stdio.h>` の使用 | CLI 入出力, ログ出力, ファイル I/O (`calc/`, `lib/fileio.c`, `lib/log.c`) |
| 6 | Rule 21.5 | Rule 20.8 | Required | シグナル処理 `<signal.h>` の使用 | デーモン制御 (SIGHUP), 割り込み (SIGINT), SIGPIPE 抑止 (`lib/sig.c`, `server/main.c`, `client/main.c`, `calc/main.c`) |
| 7 | Rule 21.8 | Rule 20.11 | Required | プロセス終了関数 (`exit()`, `atexit()`) の使用 | CLI オプション終了, 致命的エラー終了, 終了ハンドラ登録 |
| 8 | Rule 17.1 | Rule 16.1 | Required | 可変長引数 `<stdarg.h>` の使用 | ログ出力マクロ, 関数引数解析, 一括解放 (`memfree`, `close_fd`) |
| 9 | Rule 18.4, 11.3 | Rule 17.4, 11.4 | Advisory / Required | ポインタ演算およびポインタキャスト | 文字列走査, 送受信オフセット, ソケット構造体キャスト |
| 10 | Rule 14.1, 21.10 | - | Required | 浮動小数点数 (`double`) および数学関数の使用 | 電卓コア計算処理 (`calc/func.c`, `calc/calc.c`) |
| 11 | Rule 15.5 | Rule 14.7 | Advisory / Required | 早期リターン（単一終了点規則の例外） | 関数の引数検証・ガード節 |
| 12 | Rule 22.8-22.10 | - | Required | `errno` の参照 | システムコールのリトライ判定 (`EINTR`, `EAGAIN`) |
| 13 | Rule 12.3 | Rule 12.10 | Advisory / Required | カンマ演算子の使用 | 数値パースの局所的ループ制御 (`calc/calc.c`) |
| 14 | Rule 19.2 | Rule 18.4 | Advisory / Required | 共用体 (`union`) の使用 | 組み込み関数テーブル (`calc/func.c`) |
| 15 | Rule 21.10 | Rule 20.12 | Required | 標準ライブラリの日時機能 `<time.h>` の使用 | ログの時刻表示 (`lib/log.c`) |
| 16 | Rule 21.12 | - | Required | 浮動小数点例外の機能 `<fenv.h>` の使用 | 計算エラーの検出 (`calc/error.c`) |

---

## 3. 各例外事項の詳細と安全対策 (Deviations and Mitigations)

### 例外 1: goto 文によるエラーハンドラの一元化
- **該当ルール**: MISRA C:2012 Rule 15.1 (Advisory), MISRA C:2004 Rule 14.4 (Required)
  - 「`goto` 文を使用すべきではない（使用してはならない）」
- **理由 (Rationale)**:
  - プロジェクトの設計原則（README.md「コーディングのコンセプト 4. エラーハンドラでは, `goto` 文を積極的に使用する」）に基づきます。
  - エラー発生時のリソース解放（メモリ解放、ソケット・ファイルディスクリプタのクローズ等）を関数末尾の単一ラベル（`error_handler:` 等）に集約することで、解放漏れや多重解放、コードの重複を防ぎます。
  - また、深いネストによるアローアンチパターン（可読性低下）を抑制します。
- **安全対策 (Mitigation)**:
  - ジャンプ先は同一関数内の後方に配置されたエラー処理ブロック（前方ジャンプのみ）に厳格に限定します。
  - 後方ジャンプによるループ形成や、別ブロックの内部へのジャンプは一切行いません（MISRA C:2012 Rule 15.2, 15.3 に実質準拠）。

### 例外 2: 動的ヒープメモリ確保・解放 (malloc, calloc, realloc, free, strdup)
- **該当ルール**: MISRA C:2012 Rule 21.3 (Required), MISRA C:2004 Rule 20.4 (Required)
  - 「`<stdlib.h>` の動的メモリ確保・解放関数を使用してはならない」
- **理由 (Rationale)**:
  - ユーザーから入力される任意の長さの計算式文字列の解析、可変長の計算結果バッファ生成、ネットワーク送受信パケットの動的構築、マルチスレッド環境におけるクライアント情報構造体の生成に動的メモリ確保が不可欠です。
- **安全対策 (Mitigation)**:
  - メモリ確保の戻り値が `NULL` であるかのチェックを必ず行い、失敗時は適切にエラーログを出力して呼び出し元へ通知します。
  - メモリ解放時は専用ラッパー関数 `memfree()` を使用し、`free()` 直後に必ずポインタへ `NULL` を代入します（「コーディングのコンセプト 5」）。これによりダングリングポインタ参照や二重解放を防止します。
  - 単体テストにおいてメモリ確保失敗のエラー注入テストを実施し、メモリリークが発生しないことを検証しています。

### 例外 3: 関数の再帰呼び出し (Recursive Functions)
- **該当ルール**: MISRA C:2012 Rule 17.2 (Required), MISRA C:2004 Rule 16.2 (Required)
  - 「関数は直接的にも間接的にも自分自身を呼び出してはならない」
- **理由 (Rationale)**:
  - 電卓の構文解析処理 (`calc/calc.c`) において、数式の文脈自由文法を自然かつ直感的に解析するため「再帰下降構文解析法 (Recursive Descent Parsing)」を採用しています。
  - 括弧のネスト、演算子の優先順位、組み込み関数引数の解析において、`expression()` ↔ `term()` ↔ `factor()` / `parse_func_args()` 間の相互間接再帰が必須です。
- **安全対策 (Mitigation)**:
  - 入力文字列長の上限管理、および構文エラー検出時の即時中断・エラーコード設定により、スタックオーバーフローや無限再帰を防止しています。

### 例外 4: POSIX 拡張・OS 固有 API の使用 (言語拡張・非標準機能)
- **該当ルール**: MISRA C:2012 Rule 1.2 (Advisory), MISRA C:2004 Rule 1.1 / 1.2 (Required)
  - 「言語拡張を使用してはならない。未定義・未規定の動作に依存してはならない」
- **理由 (Rationale)**:
  - 本システムは Linux/POSIX OS 上で動作するネットワーク計算サービスおよび対話型 CLI アプリケーションです。
  - Berkeley ソケット API (`sys/socket.h`), POSIX スレッド (`pthread.h`), I/O 多重化 (`sys/select.h`, `poll.h`), 端末属性制御 (`termios.h`), システムログ (`syslog.h`), プロセス制御 (`fork`, `execvpe`, `waitpid`), GNU readline 等の OS 固有機能が要件上不可欠です。
- **安全対策 (Mitigation)**:
  - POSIX 標準規格に準拠した API 呼出に統一し、コンパイラ拡張等の非標準構文は極力排除しています。
  - すべてのシステムコール戻り値を検証し、環境による動作差異を吸収しています。

### 例外 5: 標準入出力ライブラリ stdio.h の使用
- **該当ルール**: MISRA C:2012 Rule 21.6 (Required), MISRA C:2004 Rule 20.9 (Required)
  - 「標準ライブラリの入出力関数 (`<stdio.h>`) を使用してはならない」
- **理由 (Rationale)**:
  - コンソールアプリケーションとしてのユーザ対話、標準エラー出力へのエラー表示、パイプ・リダイレクト処理、ファイル入出力、文字列バッファへの整形出力に必須です。
- **安全対策 (Mitigation)**:
  - バッファ境界を見ない危険な関数（`gets`, `sprintf`, `strcpy` 等）を全面的に禁止。
  - 出力サイズの上限を厳密に指定する `snprintf` のみを使用し、境界オーバーフローを防止しています。

### 例外 6: シグナル処理ライブラリ signal.h の使用
- **該当ルール**: MISRA C:2012 Rule 21.5 (Required), MISRA C:2004 Rule 20.8 (Required)
  - 「標準ヘッダファイル `<signal.h>` の機能を使用してはならない」
- **理由 (Rationale)**:
  - UNIX サーバデーモン (`calcd`) の SIGHUP による設定再読み込み・再起動、SIGTERM によるグレースフルシャットダウン、CLI (`calcc`, `calcp`) での Ctrl+C (SIGINT) 入力破棄、切断ソケットへの書き込みによる SIGPIPE 抑止にシグナル制御が必要です。
- **安全対策 (Mitigation)**:
  - シグナルハンドラ内では非同期シグナル安全 (async-signal-safe) な最小限の処理（`sig_handled` や `hupflag` 等のフラグ更新のみ）を行い、複雑な処理はメインループ側で安全に検知して実行します。

### 例外 7: プロセス終了関数 (exit(), atexit()) の使用
- **該当ルール**: MISRA C:2012 Rule 21.8 (Required), MISRA C:2004 Rule 20.11 (Required)
  - 「`<stdlib.h>` の `abort`, `exit`, `getenv`, `system` を使用してはならない」
- **理由 (Rationale)**:
  - CLI コマンドの `--help` や `--version` 表示後の正常終了 (`exit(EXIT_SUCCESS)`)、引数エラーやデーモン起動失敗等の致命的エラー時の終了ステータス返却 (`exit(EXIT_FAILURE)`)、`fork()` 後の子プロセス終了、および `atexit()` によるプロセス終了時リソース解放ハンドラ登録に必要です。
- **安全対策 (Mitigation)**:
  - `exit()` の呼び出し箇所はオプション解析部やプロセス起動時の初期化パスに限定し、業務ロジックの深部では使用せず戻り値（`EX_OK` / `EX_NG`）により制御します。

### 例外 8: 可変長引数 stdarg.h の使用
- **該当ルール**: MISRA C:2012 Rule 17.1 (Required), MISRA C:2004 Rule 16.1 (Required)
  - 「`<stdarg.h>` の機能を使用してはならない」
- **理由 (Rationale)**:
  - 書式付きログ出力マクロ (`dbglog`, `outlog`), 電卓の可変引数組み込み関数解析 (`parse_func_args`), 可変個ポインタの一括解放 (`memfree`), 可変個ファイルディスクリプタの一括クローズ (`close_fd`) の実装に必要です。
- **安全対策 (Mitigation)**:
  - `memfree` や `close_fd` では末尾に `NULL` 終端子を渡す呼び出し規約を徹底し、可変引数の境界外アクセスを防止しています。ログ出力では `vsnprintf` によるバッファ長制限を厳守しています。

### 例外 9: ポインタ演算およびポインタ型のキャスト
- **該当ルール**: MISRA C:2012 Rule 18.4 (Advisory), Rule 11.3 (Required), Rule 11.5 (Advisory), MISRA C:2004 Rule 17.4 (Required), Rule 11.4 (Required)
  - 「ポインタに対する演算子 (`+`, `-`, `+=`, `-=`) を適用してはならない。異なるオブジェクトポインタ間や `void *` からのキャストを行ってはならない」
- **理由 (Rationale)**:
  - 数式文字列のポインタ走査 (`calc->ptr++`)、ソケット送受信における部分送受信ループでのオフセット移動 (`ptr += len`)、および Berkeley ソケット API のアドレス構造体キャスト (`struct sockaddr *`) や汎用ポインタ引数 (`memfree()` に渡すポインタ変数のアドレス) の扱いに必要です。
- **安全対策 (Mitigation)**:
  - 残りバイト数 (`left`) や終端文字 (`'\0'`) を常に管理し、バッファ境界外アクセスを厳格に防止しています。また、構造体配置のアライメント要件（`ALIGN8` 等）を考慮しています。

### 例外 10: 浮動小数点数 (double) および数学関数の使用
- **該当ルール**: MISRA C:2012 Rule 14.1 (Required), Rule 21.10 (Required)
  - 「ループカウンタは本質的に浮動小数点型であってはならない。複素数や特定の数学関数の制約」
- **理由 (Rationale)**:
  - 本ソフトウェアの根幹機能が「科学技術計算電卓」であり、`double` 型による高精度浮動小数点演算および標準数学ライブラリ（三角関数、対数、指数、平方根、階乗、順列、組合せ等）の直接利用がコア要件です。
- **安全対策 (Mitigation)**:
  - C99 浮動小数点マクロ（`fpclassify`, `isless`, `isgreater` 等）を用い、NaN、無限大 (INFINITY)、ゼロ除算、アンダーフロー、定義域外エラーを徹底的に事前・事後検知し、適切にエラー通知（`set_errorcode()`）を行っています。

### 例外 11: 関数の複数 return 文 (早期リターン / Guard Clauses)
- **該当ルール**: MISRA C:2012 Rule 15.5 (Advisory), MISRA C:2004 Rule 14.7 (Required)
  - 「関数は末尾に単一の終了点（return 文）を持たなければならない」
- **理由 (Rationale)**:
  - 関数の入口における引数の `NULL` チェックや境界値チェックにおいて早期リターン（ガード節）を採用することで、無駄なネストの深化（インデント肥大化）を防ぎ、コードの可読性と保守性を高めています。
- **安全対策 (Mitigation)**:
  - 動的リソース確保前の事前チェックにおいてのみ早期リターンを許容し、リソース確保後は `goto error_handler;` を用いて一元的に解放・終了パスを通過させます。

### 例外 12: errno の参照
- **該当ルール**: MISRA C:2012 Rule 22.8, 22.9, 22.10 (Required)
  - 「`errno` の直接使用や参照に関する制約」
- **理由 (Rationale)**:
  - `send()`, `recv()`, `select()`, `close()` などの POSIX システムコールが失敗した際、シグナル割り込み (`EINTR`) や非ブロッキングリトライ (`EAGAIN`, `EWOULDBLOCK`) を判別して適切に再試行を行うために必須です。
- **安全対策 (Mitigation)**:
  - システムコール呼び出しの直後に `errno` を参照・評価し、後続の処理による値の上書きを避けています。

### 例外 13: カンマ演算子の使用
- **該当ルール**: MISRA C:2012 Rule 12.3 (Advisory), MISRA C:2004 Rule 12.10 (Required)
  - 「カンマ演算子を使用してはならない」
- **理由 (Rationale)**:
  - `for` 文の初期化と更新式で、複数の変数を、まとめて、初期化・更新するために使っています (`calc/func.c` の `for (i = 0, exec = false; ...)`、`lib/log.c` のダンプの `for (...; i++, j++)`)。
  - (`calc/calc.c` の `number()` にあった、`while (readch(calc), ...)` は、`readch()` を条件式の外に出して、なくしました。)
- **安全対策 (Mitigation)**:
  - 使うのは `for` 文の、初期化と更新の式だけで、条件式の中には書きません。副作用の順序は、左から右で、明確です。

### 例外 14: 共用体 (union) の使用
- **該当ルール**: MISRA C:2012 Rule 19.2 (Advisory), MISRA C:2004 Rule 18.4 (Required)
  - 「共用体を使用してはならない」
- **理由 (Rationale)**:
  - 組み込み関数 (`sin`, `sqrt`, `nPr` など) は、引数の数 (0 個、1 個、2 個) と、標準の数学関数かどうかで、関数ポインタの型が違います。これらを 1 つのテーブル (`finfo[]`) で管理するために、関数ポインタの共用体 (`union func`) を使っています (`calc/func.c`)。
- **安全対策 (Mitigation)**:
  - 共用体は、必ず、種別を表す列挙体 (`enum uniontype`) とセットにした構造体 (`struct funcinfo`) の中に置きます (タグ付き共用体)。
  - 関数を呼び出すときは、種別で `switch` して、対応するメンバだけを使います。別のメンバとして読み出しません。

### 例外 15: 標準ライブラリの日時機能 time.h の使用
- **該当ルール**: MISRA C:2012 Rule 21.10 (Required), MISRA C:2004 Rule 20.12 (Required)
  - 「標準ライブラリの日時機能 (`<time.h>`) を使用してはならない」
- **理由 (Rationale)**:
  - ログに、時刻 (月日、時分秒、マイクロ秒) を出力するために、`gettimeofday()` と `localtime_r()`、`struct tm` を使います (`lib/log.c`, `lib/timer.h`)。
- **安全対策 (Mitigation)**:
  - スレッドセーフな `localtime_r()` を使い (`localtime()` は使いません)、戻り値を必ず検証します。
  - 月の名前は、範囲を確認してから、配列を引きます。

### 例外 16: 浮動小数点例外の機能 fenv.h の使用
- **該当ルール**: MISRA C:2012 Rule 21.12 (Required)
  - 「`<fenv.h>` の例外処理の機能を使用してはならない」
- **理由 (Rationale)**:
  - 計算結果が、ゼロ除算、オーバーフロー、無効な演算 (NaN) になったことを、検出するために、`feclearexcept()` と `fetestexcept()` を使います (`calc/error.c`)。数学関数 (`log`, `sqrt` など) の定義域エラーを、確実に捕まえるには、この機能が必要です。
- **安全対策 (Mitigation)**:
  - 計算の前に、例外フラグを `feclearexcept(FE_ALL_EXCEPT)` で消し、計算の後で、必要なフラグだけを `fetestexcept()` で調べます。
  - 浮動小数点の値は、`isnan()` / `isinf()` / `fpclassify()` でも検査します (例外 10 も参照)。

---

## 4. 例外としていないもの (参考)

次の機能は、コードで使っていない (または、規約で禁止している) ため、例外に該当しません。使う必要が出たときは、このドキュメントに例外として追加してください。

| 機能 | MISRA C:2012 | 備考 |
|:---|:---|:---|
| `setjmp()` / `longjmp()` | Rule 21.4 | 非局所的なジャンプは、使わない |
| `atoi()` / `atof()` / `atol()` | Rule 21.7 | 数値への変換は、`strtol()` で、エラーと範囲を検査して行う |
| `system()` / `getenv()` / `abort()` | Rule 21.8 | 使わない (`exit()` / `atexit()` は、例外 7) |
| `<tgmath.h>` | Rule 21.11 | 型ごとの数学関数を、明示して使う |
| `strcpy()` / `strcat()` / `sprintf()` / `gets()` | Rule 21.6 の趣旨 | 長さを見ない関数は禁止 (規約。`snprintf()` などを使う) |
| `#undef` | Rule 20.5 | マクロを、定義し直さない |
| 可変長配列 (VLA) | Rule 18.8 | 配列のサイズは、定数にする (`-Wvla` で検出) |
| `alloca()` | - | 使わない (`-Walloca` で検出) |
| ビットフィールド | Rule 6.1 | 使わない |
| インラインアセンブラ (`asm`) | Dir 4.3 | 使わない |

> 上の表の「使わない」は、`git grep` などで、ソース (`lib/` `calc/` `server/` `client/`) に、該当の記述がないことを確認した結果です。

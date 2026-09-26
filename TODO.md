# TODO

## 未対応

- [ ] GNU readline から editline を使用するよう変更する. (2011-09-21)
- [ ] 多倍長整数に対応する. GNU gmp, GNU mpfr. (2011-09-21)
- [ ] デバッグビルド (`_DEBUG`) の `dbglog()` (`system_dbg_log`) が, `errno` を 0 にする.
  `lib/net.c` の `send_data()` `recv_data()` は, `dbglog()` の直後に `errno` (EINTR, EAGAIN) を判定するので, デバッグビルドでは, 割り込まれたときの再試行が働かず, エラーになる.
  `tests/lib/test_net.c` の `test_send_data_interrupted` `test_recv_data_interrupted` は, デバッグビルドでは, スキップしている.
- [ ] テストのカバレッジの残りは, 分岐 (現在 95.7%). 行は 100% (`make coverage` で確認できる). `calc/func.c` の到達しない `default:` は, `GCOVR_EXCL_START` で, 集計から外している.
- [ ] 標準出力をリダイレクトしている間にテストが失敗すると, メッセージが見えなくなる. (`tests/calcc/test_client.c` など)

## 対応済み

- [x] cutter から greatest と FFF に移行し, テストを `tests/` 配下に移した.
- [x] `main.c` と `option.c` のテストを追加した.
- [x] libc の関数の失敗を, FFF のモックで注入して, エラー処理のテストを追加した. (行 82% → 99.5%)
- [x] CMake でビルドできるようにした.
- [x] `lib/timer.h` の関数を `static inline` にした.
- [x] `calcp` が, 標準入力が端末でないとき, EOF で終了しない不具合を修正した. (端末のときだけ readline を使う. 履歴は `stifle_history` で制限する)
- [x] 全ソースのレビューで見つかった不具合を修正した. (`n()` の無限ループ, 単項の符号と `^` の優先順位, `_readline` の最後の行, `client_loop` の `FD_ISSET`, `server_proc` のリークと確保サイズと終端, `set_port` の範囲, `calcd` の SIGHUP 再起動, `calcc` の SIGPIPE, `set_block` `check_math_feexcept` `stop_timer` `sys_print_termattr` `dump_file`)
- [x] ビルドの警告を解消した. (`calc/func.c` の `-Wswitch-unreachable`, `tests/calcc/thread_client.c` の `strncat`)

# TODO

## 未対応

- [ ] GNU readline から editline を使用するよう変更する. (2011-09-21)
- [ ] 多倍長整数に対応する. GNU gmp, GNU mpfr. (2011-09-21)
- [ ] `calcp` が, 標準入力が端末でないとき (パイプやファイル), 入力の終わり (EOF) で終了せず, CPU を 100% 使い続ける.
  readline 8.3 と `rl_event_hook` (`calc/main.c` の `check_state`) の組み合わせが原因と思われる. `quit` / `exit` では正常に終了する.
  `tests/calcp/test_main.c` は, この不具合を避けるため, 入力を必ず `quit` で終わらせている.
- [ ] デバッグビルド (`_DEBUG`) の `dbglog()` (`system_dbg_log`) が, `errno` を 0 にする.
  `lib/net.c` の `send_data()` `recv_data()` は, `dbglog()` の直後に `errno` (EINTR, EAGAIN) を判定するので, デバッグビルドでは, 割り込まれたときの再試行が働かず, エラーになる.
  `tests/lib/test_net.c` の `test_send_data_interrupted` `test_recv_data_interrupted` は, デバッグビルドでは, スキップしている.
- [ ] テストのカバレッジを上げる. (`make coverage` で確認できる. 現在は, 行 93.2%, 分岐 83.3%)
  - 残りは, `main.c` の `sigaction` `atexit` `setvbuf` の失敗, `calc/calc.c` の `snprintf` `malloc` の失敗, `calc/main.c` の `check_state` (readline のイベントフック), `server/server.c` の `malloc` の失敗など.
  - `malloc` のモックは, ほとんどの関数が使うので, 素通し (`TEST_PASSTHROUGH`) にする場合は, `dlsym` が `malloc` を呼ばないか, 確認が必要.
- [ ] 標準出力をリダイレクトしている間にテストが失敗すると, メッセージが見えなくなる. (`tests/calcc/test_client.c` など)

## 対応済み

- [x] cutter から greatest と FFF に移行し, テストを `tests/` 配下に移した.
- [x] `main.c` と `option.c` のテストを追加した.
- [x] libc の関数の失敗を, FFF のモックで注入して, エラー処理のテストを追加した. (行 82% → 93%)
- [x] CMake でビルドできるようにした.
- [x] `lib/timer.h` の関数を `static inline` にした.
- [x] ビルドの警告を解消した. (`calc/func.c` の `-Wswitch-unreachable`, `tests/calcc/thread_client.c` の `strncat`)

; RUN: %opt %loadlibs -meminstrument -mi-ipolicy=access-only -mi-wstrategy=none -mi-imechanism=rt_stat -S %s > %t1.ll
; RUN: %clink -ldl -l:librt_stat.a -o %t2 %t1.ll
; RUN: %t2 2> %t3.stats
; RUN: fgrep "normal loads : 1" %t3.stats
; RUN: fgrep "normal stores : 1" %t3.stats

define i32 @main() {
test_bb:
  %p = alloca i32, i64 12

  %p2 = call i32* @foo(i32* %p)

  %r = load i32, i32* %p2
  %d = sub i32 %r, 42
  ret i32 %d
}

define i32* @foo(i32* %p) {
foo_bb:
  store i32 42, i32* %p
  ret i32* %p
}

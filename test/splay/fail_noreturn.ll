; RUN: %opt %loadlibs -meminstrument %s -mi-config=splay -S > %t1.ll
; RUN: fgrep "attributes #0 = { noreturn }" %t1.ll
; RUN: fgrep "__splay_fail_simple() #0" %t1.ll

define i32 @test(i64 %n, i32* %p) {
bb:
  %p1 = alloca i32, i64 8
  %p2 = getelementptr i32, i32* %p1, i64 4
  %x = load i32, i32* %p2
  ret i32 %x
}


; RUN: %opt -load %passlib -memsafety-genchecks %s -memsafety-imechanism=splay -S > %t1.ll
; RUN: fgrep "call void @__splay_alloc" %t1.ll

define i32 @test(i64 %n, i32* %p) {
bb:
  %p1 = alloca i32, i64 8
  %p2 = getelementptr i32, i32* %p1, i64 4
  %x = load i32, i32* %p2
  ret i32 %x
}


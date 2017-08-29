; RUN: %opt -load %passlib -memsafety-genchecks %s -memsafety-imechanism=splay -S > %t1.ll
; RUN: %clink -ldl -lsplay -o %t2 %t1.ll
; RUN: %t2

define i32 @foo() {
bb:
  %p1 = alloca i32, i64 8
  %p2 = getelementptr i32, i32* %p1, i64 4
  %x = load i32, i32* %p2
  ret i32 %x
}

define i32 @main() {
bb:
  %v1 = call i32 @foo()
  %v2 = call i32 @foo()
  ret i32 0
}


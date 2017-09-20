; RUN: %opt %loadlibs -mi-genwitnesses -S %s > %t.ll
; RUN: %not fgrep "i_witness" %t.ll

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @foo(i1 %cond) {
entry:
  %a = alloca i32, i64 7, align 4

  %p1 = getelementptr inbounds i32, i32* %a, i64 1
  %p2 = getelementptr inbounds i32, i32* %a, i64 2

  %i = select i1 %cond, i32* %p1, i32* %p2
  %v = load i32, i32* %i, align 4

  ret i32 %v
}


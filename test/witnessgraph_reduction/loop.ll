; RUN: %opt %loadlibs -meminstrument -mi-config=splay -mi-mode=genwitnesses -S %s > %t.ll
; RUN: %not fgrep "i_witness" %t.ll

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @foo() {
entry:
  %a = alloca i32, i64 7, align 4
  br label %loop

loop:
  %i = phi i32* [%a, %entry], [%p, %loop]
  %v = load i32, i32* %i, align 4
  %p = getelementptr inbounds i32, i32* %i, i64 1
  %cond = icmp slt i32 %v, 7
  br i1 %cond, label %loop, label %exit

exit:
  ret i32 0
}


; RUN: %opt %loadlibs -meminstrument -S -mi-config=softbound -mi-mode=gatheritargets -debug-only=meminstrument-itargetprovider %s 2>&1 | %fileCheck %s

; CHECK: dereference check with constant size 4B for p at loop::[store,2]
; CHECK: dereference check with constant size 4B for a at exit

; REQUIRES: asserts

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @foo() {
entry:
  %a = alloca i32, i64 7, align 4
  br label %loop

loop:
  %i = phi i64 [0, %entry], [%ip, %loop]
  %p = phi i32* [%a, %entry], [%pp, %loop]
  store i32 42, i32* %p, align 4
  %ip = add nsw i64 %i, 1
  %pp = getelementptr inbounds i32, i32* %p, i64 1
  %cond = icmp slt i64 %i, 7
  br i1 %cond, label %loop, label %exit

exit:
  %tmp = load i32, i32* %a, align 4
  ret i32 %tmp
}

; RUN: %opt %loadlibs -memsafety-genchecks %s -memsafety-imechanism=splay -S > %t1.ll
; RUN: fgrep "call void @__splay_alloc_or_merge(i8* bitcast (i32* @g to i8*), i64 4)" %t1.ll

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@g = global i32 42

define i32 @foo() {
entry:
  %tmp = load i32, i32* @g, align 4
  ret i32 %tmp
}


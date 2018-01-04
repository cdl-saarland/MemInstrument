; RUN: %opt %loadlibs -meminstrument %s -mi-config=splay -S > %t1.ll
; RUN: grep -e "call void @__splay_alloc_or_merge(.*, i64 13)" %t1.ll
; RUN: %not fgrep "call void @__splay_check" %t1.ll

; ModuleID = 'hello_world.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [13 x i8] c"Hello World!\00", align 1

define i32 @main() #0 {
  ret i32 0
}

declare i32 @puts(i8*) #1


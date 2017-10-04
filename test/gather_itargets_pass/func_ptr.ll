; RUN: %opt %loadlibs -mi-itargetprovider -debug-only=meminstrument-itargetprovider %s > /dev/null 2> %t.log
; RUN: fgrep "<a, bar_bb::res, 1B, ___>" %t.log
; RUN: fgrep "<b, bar_bb::res, 1B, ___>" %t.log
; RUN: fgrep "<x, foo_bb::res, 1B, ___>" %t.log
; RUN: fgrep "<y, foo_bb::res, 1B, ___>" %t.log

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @bar(i32 (i8*, i8*)* nocapture %arg, i8* %a, i8* %b) {
bar_bb:
  %res = call i32 %arg(i8* %a, i8* %b)
  ret i32 %res
}

declare i32 @baz(i8*, i8*)

define i32 @foo() {
foo_bb:
  %x = alloca i8, i64 4
  %y = alloca i8, i64 8
  %res = call i32 @bar(i32 (i8*, i8*)* nonnull @baz, i8* %x, i8* %y)
  ret i32 %res
}


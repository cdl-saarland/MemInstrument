; RUN: %opt %loadlibs -meminstrument -mi-config=splay -mi-mode=gatheritargets -debug-only meminstrument-itargetprovider %s -S > /dev/null 2> %t1.log
; RUN: %opt %loadlibs -meminstrument -mi-config=splay -mi-mode=filteritargets -debug-only meminstrument-itargetfilter %s -S > /dev/null 2> %t2.log
; RUN: fgrep "<dereference check with constant size 4B for arrayidx at entry::[store,3]>" %t1.log
; RUN: fgrep "<dereference check with constant size 4B for arrayidx1 at entry::bar>" %t1.log
; RUN: %not fgrep "<dereference check with constant size 4B for arrayidx at entry::[store,3]>" %t2.log
; RUN: fgrep "<dereference check with constant size 4B for arrayidx1 at entry::bar>" %t2.log

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define i32 @main() {
entry:
  %call = call noalias i8* @malloc(i64 24)
  %foo = bitcast i8* %call to i32*
  %arrayidx = getelementptr inbounds i32, i32* %foo, i64 3
  store i32 42, i32* %arrayidx, align 4, !nosanitize !1
  %arrayidx1 = getelementptr inbounds i32, i32* %foo, i64 6
  %bar = load i32, i32* %arrayidx1, align 4
  ret i32 %bar
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64)

!1 = !{i32 1}

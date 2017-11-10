; RUN: %opt %loadlibs -meminstrument -mi-config=splay -mi-mode=gatheritargets -debug-only meminstrument-itargetprovider %s -S > /dev/null 2> %t1.log
; RUN: %opt %loadlibs -meminstrument -mi-config=splay -mi-mode=filteritargets -debug-only meminstrument-itargetfilter %s -S > /dev/null 2> %t2.log
; RUN: fgrep "if.then::foo" %t1.log
; RUN: %not fgrep "if.then::foo" %t2.log

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@A = common global [15 x i32] zeroinitializer, align 16

; Function Attrs: noinline nounwind uwtable
define i32 @f(i32 %a, i32 %b) {
entry:
  %cmp = icmp slt i32 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %foo = load i32, i32* getelementptr inbounds ([15 x i32], [15 x i32]* @A, i64 0, i64 0), align 16, !nosanitize !1
  br label %return

if.end:                                           ; preds = %entry
  br label %return

return:                                           ; preds = %if.end, %if.then
  %retval = phi i32 [ %foo, %if.then ], [ 1, %if.end ]
  ret i32 %retval
}

!1 = !{i32 1}

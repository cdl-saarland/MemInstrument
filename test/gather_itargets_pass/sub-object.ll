; RUN: %opt %loadlibs -memsafety-gatheritargets -debug-only=meminstrument-gatheritargets %s > /dev/null 2> %t.log
; RUN: fgrep "<add.ptr, entry::tmp, 4B, ul_>" %t.log
; RUN: fgrep "<arrayidx, for.body::[store], 4B, ul_>" %t.log
; RUN: fgrep "<tmp, for.end::call1, 4B, ___>" %t.log
; RUN: fgrep "<i, entry::[store], 4B, ul_>" %t.log
; RUN: fgrep "<x, entry::[store], 8B, ul_>" %t.log
; RUN: fgrep "<i1, entry::[store], 4B, ___>" %t.log
; RUN: fgrep "<i2, entry::call, 4B, ___>" %t.log
; ModuleID = 'test_subobj.o0.ll'
source_filename = "test_subobj.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.anon = type { i32, i32* }

; Function Attrs: noinline nounwind uwtable
define i32 @foo(i32* %p) #0 {
entry:
  %add.ptr = getelementptr inbounds i32, i32* %p, i64 1
  %tmp = load i32, i32* %add.ptr, align 4
  ret i32 %tmp
}

; Function Attrs: noinline nounwind uwtable
define i32 @bar() #0 {
entry:
  %call = call noalias i8* @malloc(i64 28) #2
  %tmp = bitcast i8* %call to i32*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, 7
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul nsw i32 2, %i.0
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, i32* %tmp, i64 %idxprom
  store i32 %mul, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %call1 = call i32 @foo(i32* %tmp)
  ret i32 %call1
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #1

; Function Attrs: noinline nounwind uwtable
define i32 @baz() #0 {
entry:
  %s = alloca %struct.anon, align 8
  %i = getelementptr inbounds %struct.anon, %struct.anon* %s, i32 0, i32 0
  store i32 42, i32* %i, align 8
  %i1 = getelementptr inbounds %struct.anon, %struct.anon* %s, i32 0, i32 0
  %x = getelementptr inbounds %struct.anon, %struct.anon* %s, i32 0, i32 1
  store i32* %i1, i32** %x, align 8
  %i2 = getelementptr inbounds %struct.anon, %struct.anon* %s, i32 0, i32 0
  %call = call i32 @foo(i32* %i2)
  ret i32 %call
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.1 (https://github.com/llvm-mirror/clang.git 3c8961bedc65c9a15cbe67a2ef385a0938f7cfef) (https://github.com/llvm-mirror/llvm.git c8fccc53ed66d505898f8850bcc690c977a7c9a7)"}

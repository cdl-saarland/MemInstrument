; RUN: %opt %loadlibs -meminstrument -S -mi-config=softbound -mi-mode=gatheritargets -debug-only=meminstrument-itargetprovider %s 2>&1 | %fileCheck %s

; CHECK: value invariant for Multi at entry::[store,4]
; CHECK-NEXT: dereference check with constant size 8B for opt at entry::[store,4]
; CHECK-NEXT: dereference check with constant size 8B for opt1 at entry::[{{.*}},6]
; CHECK-NEXT: call check{{.*}}at entry::call
; CHECK-NEXT: call invariant{{.*}}entry::call
; CHECK-NEXT: dereference check with constant size 4B for result at entry::[store,9]
; CHECK-NEXT: dereference check with constant size 4B for result2 at entry::[{{.*}},11]
; CHECK-NEXT: call invariant{{.*}}entry::call3
; CHECK-NEXT: argument 0 invariant {{.*}}at entry::call3

; REQUIRES: asserts

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.STR = type { i32, i32 (i32, i32)* }

@.str = private unnamed_addr constant [33 x i8] c"The multiplication result is %d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @Multi(i32 %a, i32 %b) #0 {
entry:
  %mul = mul nsw i32 %a, %b
  ret i32 %mul
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
entry:
  %str_obj = alloca %struct.STR, align 8
  %opt = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 1
  store i32 (i32, i32)* @Multi, i32 (i32, i32)** %opt, align 8
  %opt1 = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 1
  %0 = load i32 (i32, i32)*, i32 (i32, i32)** %opt1, align 8
  %call = call i32 %0(i32 5, i32 3)
  %result = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 0
  store i32 %call, i32* %result, align 8
  %result2 = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 0
  %1 = load i32, i32* %result2, align 8
  %call3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([33 x i8], [33 x i8]* @.str, i64 0, i64 0), i32 %1)
  ret i32 0
}

declare dso_local i32 @printf(i8*, ...) #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (https://github.com/llvm/llvm-project.git c1a0a213378a458fbea1a5c77b315c7dce08fd05)"}

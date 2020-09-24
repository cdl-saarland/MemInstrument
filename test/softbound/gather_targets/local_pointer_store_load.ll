; RUN: %opt -mem2reg %loadlibs -meminstrument -S -mi-config=softbound -mi-mode=gatheritargets -debug-only=meminstrument-itargetprovider %s 2>&1 | %fileCheck %s

; CHECK: dereference check with constant size 4B for retval at entry::[store,3]
; CHECK-NEXT: dereference check with constant size 4B for x at entry::[store,4]
; CHECK-NEXT: value invariant for x at entry::[store,5]
; CHECK-NEXT: dereference check with constant size 8B for y at entry::[store,5]
; CHECK-NEXT: dereference check with constant size 8B for y at entry::[{{.*}},6]
; CHECK-NEXT: dereference check with constant size 4B{{.*}}at entry::[{{.*}},7]

; REQUIRES: asserts

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32*, align 8
  store i32 0, i32* %retval, align 4
  store i32 10, i32* %x, align 4
  store i32* %x, i32** %y, align 8
  %0 = load i32*, i32** %y, align 8
  %1 = load i32, i32* %0, align 4
  ret i32 %1
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (https://github.com/llvm/llvm-project.git c1a0a213378a458fbea1a5c77b315c7dce08fd05)"}

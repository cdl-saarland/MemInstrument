; RUN: %opt %loadlibs -meminstrument -S -mi-config=softbound -mi-mode=gatheritargets -debug-only=meminstrument-itargetprovider %s 2>&1 | %filecheck %s

; CHECK: value invariant{{.*}}at entry::.fca.0.insert
; CHECK-NEXT: value invariant for .fca.1.insert at {{.*}}ret
; REQUIRES: asserts

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [7 x i8] c"%p %i\0A\00", align 1
@llvm.global_ctors = appending constant [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 0, void ()* @__softboundcets_globals_setup, i8* null }], !meminstrument !0

; Function Attrs: nofree nounwind uwtable
define dso_local i32 @softboundcets_pseudo_main(i32 %argc, i8** nocapture readnone %argv) local_unnamed_addr #0 {
entry:
  %call = tail call { i32*, i32 } @f()
  %0 = extractvalue { i32*, i32 } %call, 0
  %call1 = tail call i32 (i8*, ...) @softboundcets_printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), i32* %0, i32 5)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare dso_local i32 @softboundcets_printf(i8* nocapture, ...) local_unnamed_addr #1

; Function Attrs: nofree nounwind uwtable
define dso_local { i32*, i32 } @f() local_unnamed_addr #0 {
entry:
  %call = tail call noalias i8* @softboundcets_malloc(i64 1) #2
  %0 = bitcast i8* %call to i32*
  %.fca.0.insert = insertvalue { i32*, i32 } undef, i32* %0, 0
  %.fca.1.insert = insertvalue { i32*, i32 } %.fca.0.insert, i32 5, 1
  ret { i32*, i32 } %.fca.1.insert
}

; Function Attrs: nofree nounwind
declare dso_local noalias i8* @softboundcets_malloc(i64) local_unnamed_addr #1

declare !SoftBound !3 void @__softboundcets_init() local_unnamed_addr

define internal void @__softboundcets_globals_setup() !SoftBound !4 !meminstrument !0 {
entry:
  tail call void @__softboundcets_init()
  ret void
}

attributes #0 = { nofree nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!1}
!llvm.ident = !{!2}

!0 = !{!"no_instrument"}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{!"clang version 9.0.1 (https://github.com/llvm/llvm-project.git c1a0a213378a458fbea1a5c77b315c7dce08fd05)"}
!3 = !{!"RTPrototype"}
!4 = !{!"Setup"}
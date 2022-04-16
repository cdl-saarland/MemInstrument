; RUN: %opt %loadlibs -meminstrument %s -mi-config=lowfat -S -o %t.instrumented.ll -stats 2>&1 | %filecheck %s
; RUN: %clang -mcmodel=large %t.instrumented.ll %linklowfat -o %t
; RUN: %t

; CHECK: 3{{.*}}inbounds checks inserted
; CHECK: 1{{.*}}calls with undef arguments

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [17 x i8] c"Int: %d Ptr: %p\0A\00", align 1
@a = dso_local global i32 0, align 4
@some_thing = dso_local local_unnamed_addr global [5 x i32] zeroinitializer, align 16

; Function Attrs: nofree noinline nounwind uwtable
define dso_local void @stuff(i32 %x, i32* %q) local_unnamed_addr #0 {
entry:
  %call = call i32 (i8*, ...) @printf(i8* nonnull dereferenceable(1) getelementptr inbounds ([17 x i8], [17 x i8]* @.str, i64 0, i64 0), i32 %x, i32* %q)
  ret void
}

; Function Attrs: nofree nounwind
declare dso_local noundef i32 @printf(i8* nocapture noundef readonly %0, ...) local_unnamed_addr #1

; Function Attrs: nofree noinline nounwind uwtable
define dso_local i32 @fun_that_ignores_arg(i32 returned %x, i32* nocapture readnone %p) local_unnamed_addr #0 {
entry:
  call void @stuff(i32 %x, i32* nonnull @a)
  ret i32 %x
}

; Function Attrs: nofree nounwind uwtable
define dso_local i32 @main(i32 %argc, i8** nocapture readnone %argv) local_unnamed_addr #2 {
entry:
  %call = call i32 @fun_that_ignores_arg(i32 0, i32* undef)
  ret i32 0
}

attributes #0 = { nofree noinline nounwind uwtable "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nounwind "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nofree nounwind uwtable "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"Code Model", i32 4}
!2 = !{!"clang version 12.0.1 (https://github.com/llvm/llvm-project.git fed41342a82f5a3a9201819a82bf7a48313e296b)"}

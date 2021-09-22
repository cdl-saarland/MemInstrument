; RUN: %opt %loadlibs -meminstrument -S -mi-config=softbound -mi-mode=genchecks -debug-only=softbound-genchecks %s 2>&1 | %filecheck %s

; CHECK: define {{.*}} { i32*, i32* } @f()
; CHECK: {{.*}}allocate{{.*}} 1
; CHECK-NEXT: %call = call noalias i8* @softboundcets_malloc({{.*}} 10)
; CHECK-NEXT: %sb.base.load = call i8* @__softboundcets_load_base_shadow_stack({{.*}} 0)
; CHECK-NEXT: %sb.bound.load = call i8* @__softboundcets_load_bound_shadow_stack({{.*}} 0)
; CHECK-NEXT: {{.*}}deallocate
; CHECK-NEXT: %0 = bitcast i8* %call to i32*
; CHECK-NEXT: {{.*}}allocate{{.*}} 1
; CHECK-NEXT: %call1 = call noalias i8* @softboundcets_malloc({{.*}} 1)
; CHECK-NEXT: %sb.base.load1 = call i8* @__softboundcets_load_base_shadow_stack({{.*}} 0)
; CHECK-NEXT: %sb.bound.load2 = call i8* @__softboundcets_load_bound_shadow_stack({{.*}} 0)
; CHECK-NEXT: {{.*}}deallocate
; CHECK-NEXT: %1 = bitcast i8* %call1 to i32*
; CHECK-NEXT: %.fca.0.insert = insertvalue { i32*, i32* } undef, i32* %0, 0
; CHECK-NEXT: %.fca.1.insert = insertvalue { i32*, i32* } %.fca.0.insert, i32* %1, 1
; CHECK-NEXT: call void @__softboundcets_store_base_shadow_stack(i8* %sb.base.load, {{.*}} 0)
; CHECK-NEXT: call void @__softboundcets_store_bound_shadow_stack(i8* %sb.bound.load, {{.*}} 0)
; CHECK-NEXT: call void @__softboundcets_store_base_shadow_stack(i8* %sb.base.load1, {{.*}} 1)
; CHECK-NEXT: call void @__softboundcets_store_bound_shadow_stack(i8* %sb.bound.load2, {{.*}} 1)
; CHECK-NEXT: ret { i32*, i32* } %.fca.1.insert

; REQUIRES: asserts

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [7 x i8] c"%p %p\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %call = call { i32*, i32* } @f()
  %0 = extractvalue { i32*, i32* } %call, 0
  %1 = extractvalue { i32*, i32* } %call, 1
  %call1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), i32* %1, i32* %0)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) #1

; Function Attrs: nounwind uwtable
define dso_local { i32*, i32* } @f() #0 {
entry:
  %call = call noalias i8* @malloc(i64 10) #2
  %0 = bitcast i8* %call to i32*
  %call1 = call noalias i8* @malloc(i64 1) #2
  %1 = bitcast i8* %call1 to i32*
  %.fca.0.insert = insertvalue { i32*, i32* } undef, i32* %0, 0
  %.fca.1.insert = insertvalue { i32*, i32* } %.fca.0.insert, i32* %1, 1
  ret { i32*, i32* } %.fca.1.insert
}

; Function Attrs: nofree nounwind
declare dso_local noalias i8* @malloc(i64) #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (https://github.com/llvm/llvm-project.git c1a0a213378a458fbea1a5c77b315c7dce08fd05)"}

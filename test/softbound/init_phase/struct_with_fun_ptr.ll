; RUN: %opt %loadlibs -meminstrument %s -mi-config=softbound -mi-mode=setup -debug-only=softbound -S 2>&1 | %filecheck %s

; CHECK: Insert metadata store
; CHECK-NEXT: Ptr: i8* bitcast (i32 (i32, i32)** getelementptr inbounds (%struct.STR, %struct.STR* @add_str, i32 0, i32 1) to i8*)
; CHECK-NEXT: Base: i8* bitcast (i32 (i32, i32)* @Add to i8*)
; CHECK-NEXT: Bound: i8* bitcast (i32 (i32, i32)* @Add to i8*)

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.STR = type { i32, i32 (i32, i32)* }

@add_str = dso_local global %struct.STR { i32 5, i32 (i32, i32)* @Add }, align 8
@.str = private unnamed_addr constant [27 x i8] c"The addition result is %d\0A\00", align 1
@.str.1 = private unnamed_addr constant [33 x i8] c"The multiplication result is %d\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @Add(i32 %a, i32 %b) #0 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @Multi(i32 %a, i32 %b) #0 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %mul = mul nsw i32 %0, %1
  ret i32 %mul
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 8
  %str_obj = alloca %struct.STR, align 8
  store i32 0, i32* %retval, align 4
  store i32 %argc, i32* %argc.addr, align 4
  store i8** %argv, i8*** %argv.addr, align 8
  %0 = load i32 (i32, i32)*, i32 (i32, i32)** getelementptr inbounds (%struct.STR, %struct.STR* @add_str, i32 0, i32 1), align 8
  %call = call i32 %0(i32 5, i32 3)
  store i32 %call, i32* getelementptr inbounds (%struct.STR, %struct.STR* @add_str, i32 0, i32 0), align 8
  %1 = load i32, i32* getelementptr inbounds (%struct.STR, %struct.STR* @add_str, i32 0, i32 0), align 8
  %call1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([27 x i8], [27 x i8]* @.str, i64 0, i64 0), i32 %1)
  %opt = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 1
  store i32 (i32, i32)* @Multi, i32 (i32, i32)** %opt, align 8
  %opt2 = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 1
  %2 = load i32 (i32, i32)*, i32 (i32, i32)** %opt2, align 8
  %call3 = call i32 %2(i32 5, i32 3)
  %result = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 0
  store i32 %call3, i32* %result, align 8
  %result4 = getelementptr inbounds %struct.STR, %struct.STR* %str_obj, i32 0, i32 0
  %3 = load i32, i32* %result4, align 8
  %call5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([33 x i8], [33 x i8]* @.str.1, i64 0, i64 0), i32 %3)
  ret i32 0
}

declare dso_local i32 @printf(i8*, ...) #1

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (https://github.com/llvm/llvm-project.git c1a0a213378a458fbea1a5c77b315c7dce08fd05)"}

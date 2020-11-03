; RUN: %opt %loadlibs -meminstrument %s -mi-config=softbound -S 2>&1 | %fileCheck %s

; CHECK: %0 = call i8* @__softboundcets_load_base_shadow_stack(i32 0)
; CHECK-NEXT: %1 = call i8* @__softboundcets_load_bound_shadow_stack(i32 0)
; CHECK: %p1 = getelementptr i32, i32* %p, i64 0
; CHECK-NEXT: %p1_casted = bitcast i32* %p1 to i8*
; CHECK-NEXT: call void @__softboundcets_spatial_dereference_check(i8* %0, i8* %1, i8* %p1_casted, i64 4)
; CHECK-NEXT: load {{.*}} i32* %p1

define i32 @test(i32* %p) {
bb:
  %p1 = getelementptr i32, i32* %p, i64 0
  %x1 = load i32, i32* %p1
  ret i32 42
}

// RUN: %clang -c -S -Xclang -load -Xclang %passlib -O1  %s -mllvm -mi-config=softbound -mllvm -mi-mode=genwitnesses -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: Insert witness {{.*}}intermediate target for y at entry
// CHECK: Base{{.*}} @y to i8*
// CHECK-NEXT:  Bound{{.*}} getelementptr inbounds (i32*, i32** @y, i32 1

// CHECK: Insert witness {{.*}}intermediate target at entry::[ret,1]
// CHECK-NEXT: Instrumentee{{.*}} load i32*, i32** @y
// CHECK-NEXT: Insert metadata load:
// CHECK-NEXT: Ptr: {{.*}} @y
// CHECK-NEXT: BaseAlloc
// CHECK-NEXT: BoundAlloc
// CHECK-NEXT: Created bounds:
// CHECK-NEXT: Base: {{.*}} load {{.*}} %base.alloc
// CHECK-NEXT: Bound: {{.*}} load {{.*}} %bound.alloc

// CHECK: Insert witness {{.*}}intermediate target for call at entry
// CHECK-NEXT: Instrumentee{{.*}} call {{.*}}@f
// CHECK-NEXT: Created bounds:
// CHECK-NEXT: Base{{.*}} @__softboundcets_load_base_shadow_stack(i32 0), !SoftBound !3
// CHECK-NEXT: Bound{{.*}} @__softboundcets_load_bound_shadow_stack(i32 0), !SoftBound !3

// REQUIRES: asserts

int x;
int *y = &x;

int *f() {
  return y;
}

int main() {
  return *f();
}

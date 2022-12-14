// RUN: %clang -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=genwitnesses -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: Insert witness {{.*}}source target for q at entry
// CHECK: Base{{.*}} @__softboundcets_load_base_shadow_stack(i32 1)
// CHECK-NEXT: Bound{{.*}}@__softboundcets_load_bound_shadow_stack(i32 1)

// CHECK: Insert witness {{.*}}source target for p at entry
// CHECK: Base{{.*}} @__softboundcets_load_base_shadow_stack(i32 0)
// CHECK-NEXT: Bound{{.*}}@__softboundcets_load_bound_shadow_stack(i32 0)

// REQUIRES: asserts

#include <stdio.h>

int a;

int f(int *p, int x, int **q) {
  x += **q;
  return *p + x;
}

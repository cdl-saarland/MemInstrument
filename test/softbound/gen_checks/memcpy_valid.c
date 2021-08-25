// RUN: %clang -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=genchecks -mllvm -debug-only=softbound-genchecks -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: value invariant for s
// CHECK-NEXT: Metadata for pointer store to memory saved.

// CHECK: call invariant for softboundcets_malloc
// CHECK-NEXT: @__softboundcets_allocate_shadow_stack_space(i32 1)
// CHECK-NEXT: @__softboundcets_deallocate_shadow_stack_space()

// CHECK: call invariant for llvm.memcpy
// CHECK-NEXT: Inserted metadata copy

// CHECK: dereference check with constant size {{.*}} with witness
// CHECK: @__softboundcets_spatial_dereference_check

// CHECK: dereference check with constant size {{.*}} with witness
// CHECK: @__softboundcets_spatial_dereference_check

// CHECK: dereference check with constant size {{.*}} with witness
// CHECK: @__softboundcets_spatial_dereference_check

// CHECK: dereference check with variable size {{.*}} with witness
// CHECK: @__softboundcets_spatial_dereference_check

// CHECK: dereference check with variable size {{.*}} with witness
// CHECK: @softboundcets_malloc
// CHECK: @__softboundcets_spatial_dereference_check

// CHECK: dereference check with constant size
// CHECK: @__softboundcets_spatial_dereference_check

// REQUIRES: asserts

#include <string.h>
#include <stdlib.h>

struct S {
  int a;
  int b;
  struct S* next;
};

int main(void) {
  struct S s;
  s.a = 42;
  s.b = 17;
  s.next = &s;

  struct S* t = malloc(sizeof(struct S));

  memcpy(t, &s, sizeof(struct S));

  return t->b - 17;
}

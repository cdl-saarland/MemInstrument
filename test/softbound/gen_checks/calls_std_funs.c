// RUN: %clang -c -S -Xclang -load -Xclang %passlib -O1  %s -mllvm -mi-config=softbound -mllvm -mi-mode=genchecks -mllvm -debug-only=softbound-genchecks  -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: call invariant for softboundcets_strcpy with 2 arg(s)
// CHECK-NEXT: __softboundcets_allocate_shadow_stack_space(i32 3)
// CHECK-NEXT: __softboundcets_deallocate_shadow_stack_space()
// CHECK-NEXT: Passed pointer information for arg 0 stored to shadow stack slot 1.
// CHECK-NEXT: Passed pointer information for arg 1 stored to shadow stack slot 2.

// CHECK: call invariant for softboundcets_atoi with 1 arg(s)
// CHECK-NEXT: @__softboundcets_allocate_shadow_stack_space(i32 1)
// CHECK-NEXT: @__softboundcets_deallocate_shadow_stack_space()

// CHECK: Passed pointer information for arg 0 stored to shadow stack slot 0.

// CHECK: call invariant for softboundcets_printf with 2 arg(s)
// CHECK-NEXT: @__softboundcets_allocate_shadow_stack_space(i32 2)
// CHECK-NEXT: @__softboundcets_deallocate_shadow_stack_space()
// CHECK-NEXT: Passed pointer information for arg 0 stored to shadow stack slot 0.
// CHECK-NEXT: Passed pointer information for arg 1 stored to shadow stack slot 1.

// REQUIRES: asserts

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int val;
  char str[20];

  strcpy(str, "123456789");
  val = atoi(str);
  printf("Str: %s, Int: %d\n", str, val);

  return 0;
}

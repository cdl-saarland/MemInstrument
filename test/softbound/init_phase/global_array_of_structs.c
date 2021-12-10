// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: Create gep for @p
// CHECK-NEXT: Indices: i32 0 i32 0 i32 0

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: {{.*}} @p
// CHECK-NEXT: Base: {{.*}} @y
// CHECK-NEXT: Bound: {{.*}} getelementptr inbounds (i32, i32* @y, i32 1)

// CHECK: Create gep for @p
// CHECK-NEXT: Indices: i32 0 i32 1 i32 0

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: {{.*}} @p
// CHECK-NEXT: Base: {{.*}} @x
// CHECK-NEXT: Bound: {{.*}} getelementptr inbounds (i32, i32* @x, i32 1)

// REQUIRES: asserts

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int *ptr;
  int x;
} Si;

int y;
int x;

Si p[2] = {{&y, 4}, {&x, 5}};

int main(int argc, char **argv) {
  printf("%i %p\n", p[1].x, p[1].ptr);
  return 0;
}

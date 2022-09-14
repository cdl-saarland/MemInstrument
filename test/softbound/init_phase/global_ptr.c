// RUN: %clang -O1 -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: Handle initializer @y
// CHECK-NEXT: Base: {{.*}} @y
// CHECK-NEXT: Bound: {{.*}} getelementptr inbounds (i32, i32* @y, i32 1)

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: i8* bitcast (i32** @Ar to i8*)

// REQUIRES: asserts

#include <stdio.h>
#include <stdlib.h>

int y;

int *Ar = &y;

int main(int argc, char **argv) {
  printf("%i\n", *Ar);
  return 0;
}

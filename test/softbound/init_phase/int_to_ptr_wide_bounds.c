// RUN: %clang -O1 -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -mi-sb-inttoptr-wide-bounds -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: i8* bitcast (i32** @y to i8*)
// CHECK-NEXT: Base: i8* null
// CHECK-NEXT: Bound: i8*

// REQUIRES: asserts

#include <stdio.h>
#include <stdlib.h>

int *y = (int *) 3;

int main(int argc, char **argv) {
  printf("%i\n", *y);
  return 0;
}

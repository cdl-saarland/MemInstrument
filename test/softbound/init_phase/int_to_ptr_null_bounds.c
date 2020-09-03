// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -sb-no-ct-error-inttoptr=1 -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: i8* bitcast (i32** @y to i8*)
// CHECK-NEXT: Base: i8* null
// CHECK-NEXT: Bound: i8* null

#include <stdio.h>
#include <stdlib.h>

int *y = (int *) 3;

int main(int argc, char **argv) {
  printf("%p\n", y);
  return 0;
}

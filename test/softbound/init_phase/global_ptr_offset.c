// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: Create bounds for: @Ar
// CHECK-NEXT: Base: @Ar = common dso_local global [15 x i32] zeroinitializer, align 16
// CHECK-NEXT: Bound: [15 x i32]* getelementptr inbounds ([15 x i32], [15 x i32]* @Ar, i32 1)

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: i8* bitcast (i32** @y to i8*)

// CHECK: Insert metadata store:
// CHECK-NEXT: Ptr: i8* bitcast (i32** @x to i8*)

#include <stdio.h>
#include <stdlib.h>

int Ar[15];

int *y = Ar + 7;
int *x = &Ar[8];

int main(int argc, char **argv) {
  printf("%i %i\n", *y, *x);
  return 0;
}

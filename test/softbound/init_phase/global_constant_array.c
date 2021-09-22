// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: Insert metadata store
// CHECK-NEXT: Ptr: i8* bitcast (i32** @p to i8*)
// CHECK-NEXT: Base: i8* bitcast ([3 x i32]* @Ar to i8*)
// CHECK-NEXT: Bound: i8* bitcast ([3 x i32]* getelementptr inbounds ([3 x i32], [3 x i32]* @Ar, i32 1) to i8*)

#include <stdio.h>
#include <stdlib.h>

int Ar[3] = {1, 2, 3};
int *p = &Ar[1];

typedef struct {
  int Ar[3];
  int ArMore[4];
} SomeArrs;

SomeArrs nums = {{1, 2, 3}, {4, 5, 6, 7}};

int main(int argc, char **argv) {
  printf("%p %i %i\n", p, nums.Ar[1], nums.ArMore[3]);
  return 0;
}

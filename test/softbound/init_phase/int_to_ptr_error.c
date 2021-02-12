// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -sb-inttoptr-disallow -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK-NOT: Insert metadata store

#include <stdio.h>
#include <stdlib.h>

int *y = (int *) 3;

int main(int argc, char **argv) {
  printf("%p\n", y);
  return 0;
}

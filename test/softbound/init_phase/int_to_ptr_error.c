// RUN: %not %clang -O1 -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -mi-sb-inttoptr-disallow -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: Meminstrument Error{{.*}}Integer to pointer cast found{{.*}}

#include <stdio.h>
#include <stdlib.h>

int *y = (int *) 3;

int main(int argc, char **argv) {
  printf("%p\n", y);
  return 0;
}

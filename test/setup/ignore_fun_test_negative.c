// RUN: %clang -O1 -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -mi-ignored-functions-file=%testfolder/setup/fileDoesNotExist.list -mllvm -stats -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK-NOT: meminstrument{{.*}}Number of functions ignored due to the given ignore file

// REQUIRES: asserts

#include <stdlib.h>

int f(int *p, int x) {
  return p[x];
}

int g(int *q, int y) {
  if (y > 0) {
    return q[y];
  }
  return 0;
}

int main() {
  int *ar = malloc(10 * sizeof(int));
  return g(ar, ar[0]) + f(ar, 5);
}

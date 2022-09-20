// RUN: %clang -fplugin=%passlib -emit-llvm -O3 -c -S -mllvm -mi-config=example -o %t.ll %s 2> %t3.log
// RUN: %clink -o %t %t.ll -ldl -l:libexample.a
// RUN: %t

#include <stdlib.h>

int foo(int *p, int N) {
  p[0] = 1;
  for (int i = 1; i < N; ++i) {
    p[i] = 2 * p[i-1];
  }
  return p[N-1];
}

int main(void) {
  int sz = 20;
  int *p = malloc(sz * sizeof(int));
  foo(p, sz);
  return 0;
}

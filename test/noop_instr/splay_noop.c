// RUN: %clang -Xclang -load -Xclang %passlib -emit-llvm -O3 -c -S -mllvm -mi-config=splay -mllvm -mi-noop-time-deref-check=10000000 -o %t0.ll %s 2> %t3.log
// RUN: %clink -ldl -l:libsleep.a -o %t1 %t0.ll
// RUN: %t1

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

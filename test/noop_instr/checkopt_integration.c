// RUN: %clang -Xclang -load -Xclang %passlib -emit-llvm -O3 -c -S -mllvm -mi-config=noop -mllvm -mi-use-extchecks -mllvm -mi-wstrategy=after-inflow -mllvm -mi-noop-time-check=10 -mllvm -mi-noop-time-gen-bounds=10 -o %t0.ll %s
// RUN: egrep "call .* @usleep" %t0.ll | wc -l | grep 2
// RUN: %clink -o %t1 %t0.ll
// RUN: %t1
// REQUIRES: pmda

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

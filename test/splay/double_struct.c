// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -mi-genchecks %t0.ll -mi-imechanism=splay -mi-splay-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <stdint.h>

struct S {
  struct T {
    double x;
    double y;
  } t;
  double z;
};

int main(void)
{
  struct S s;

  double user_time = (double) s.t.x + s.t.y/1000000.0;

  return 0;
}


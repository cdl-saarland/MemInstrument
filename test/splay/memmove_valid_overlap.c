// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -memsafety-genchecks %t0.ll -memsafety-imechanism=splay -memsafety-splay-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 2> %t3.err
// RUN: %not fgrep "non-existing" < %t3.err
// Source and destination of memmove may overlap.

#include <string.h>
#include <stdlib.h>

int main(void)
{
  int* t = malloc(sizeof(int) * 20);

  int* s = t + 4;

  memmove(t, s, sizeof(int) * 6);

  return 0;
}


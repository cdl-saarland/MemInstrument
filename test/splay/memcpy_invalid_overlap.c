// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not %t2 2> %t3.err
// RUN: fgrep "Memory safety violation" < %t3.err
// XFAIL: *
// Source and destination of memcpy should be disjoint.
// It is unclear whether we want to check this.

#include <string.h>
#include <stdlib.h>

int main(void)
{
  int* t = malloc(sizeof(int) * 20);

  int* s = t + 4;

  memcpy(t, s, sizeof(int) * 6);

  return 0;
}



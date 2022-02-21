// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument -mi-config=splay %t0.ll -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <stdlib.h>

void foo(int *A) {
  for (int i = 0; i < 4; ++i) {
    A--;
    *A = 42;
  }
}


int main(void)
{
  int *a = (int*)malloc(4*sizeof(int));
  a[4] = 42;
  return 0;
}

// Valid uses of pointers right after the end of an allocation are not yet supported
// XFAIL: *

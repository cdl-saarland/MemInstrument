// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument -mi-config=splay %t0.ll -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not %t2 2> %t3.log
// RUN: fgrep "Memory safety violation!" %t3.log

#include <stdlib.h>

int main(void)
{
  char *a = malloc(4*sizeof(char));
  a[4] = 42;
  return 0;
}


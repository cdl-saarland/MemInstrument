// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not --crash %t2 2> %t3.err
// RUN: fgrep "Memory safety violation" < %t3.err

#include <string.h>
#include <stdlib.h>

struct S {
  int a;
  int b;
  struct S* next;
};

int main(void)
{
  struct S* t = malloc(2);

  struct S s;

  memcpy(&s, t, sizeof(struct S));

  return 0;
}


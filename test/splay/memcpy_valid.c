// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 2> %t3.err
// RUN: %not fgrep "non-existing" < %t3.err

#include <string.h>
#include <stdlib.h>

struct S {
  int a;
  int b;
  struct S* next;
};

int main(void)
{
  struct S s;
  s.a = 42;
  s.b = 17;
  s.next = &s;

  struct S* t = malloc(sizeof(struct S));

  memcpy(t, &s, sizeof(struct S));

  return t->b - 17;
}


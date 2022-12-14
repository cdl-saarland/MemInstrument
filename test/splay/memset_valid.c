// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 2> %t3.err
// RUN: %not fgrep "non-existing" < %t3.err

#include <string.h>
#include <stdlib.h>

struct S {
  char a;
  char b;
  char c;
  struct S* next;
};

int main(void)
{
  struct S* t = malloc(sizeof(struct S));

  memset(t, 42, sizeof(struct S));

  return t->b - 42;
}


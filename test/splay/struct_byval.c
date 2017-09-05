// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -memsafety-genchecks %t0.ll -memsafety-imechanism=splay -memsafety-splay-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 2> %t3.err
// RUN: %not fgrep "non-existing" < %t3.err
// XFAIL: *
// This fails because of the byval parameter attribute that makes implicit copies

#include <stdint.h>

struct S {
  int a;
  int b;
  int c;
  int d;
  struct S* next;
};

int foo(struct S s) {
  s.b = 7;
  return s.a;
}

int main(void)
{
  struct S s;
  s.a = 42;
  s.b = 17;
  s.a = foo(s) + s.b;

  return 0;
}


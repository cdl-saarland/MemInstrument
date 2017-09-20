// RUN: %clang -O3 -x c++ -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -instnamer -mi-genchecks %t0.ll
// -mi-imechanism=splay -S > %t1.ll
// RUN: %clink -lstdc++ -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2
#include <cstdlib>

class Foo {
public:
  int x;
  int y;

  Foo(void) {
    x = 42;
    y = 73;
  }
};

Foo s1;
Foo s2;

int main(void) { return s1.x - s2.x; }

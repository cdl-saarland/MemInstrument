// RUN: %clang -O3 -x c++ -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -instnamer -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -lstdc++ -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 2> %t3.log
// RUN: %not fgrep "non-existing" %t3.log
// XFAIL: *

#include <typeinfo>
#include <cstdio>

int main() {
  printf("%d", typeid(int) == typeid(float));
}

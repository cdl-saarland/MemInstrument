// RUN: %clang -O3 -x c++ -emit-llvm -c -S -g -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -instnamer -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -lstdc++ -ldl -l:libsplay.a -g -o %t2 %t1.ll
// RUN: %t2
// XFAIL: *
// This fails because we cannot add the external static data required for cout
// to the splay tree.
#include <iostream>

int main(void) {
  std::cout << "Hello World!" << std::endl;
  return 0;
}

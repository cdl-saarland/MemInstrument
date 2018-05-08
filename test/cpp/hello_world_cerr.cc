// RUN: %clang -O3 -x c++ -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -instnamer -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -lstdc++ -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2
// XFAIL: *
// This fails because we cannot add the external static data required for cerr
// to the splay tree.
#include <iostream>

int main(void) {
  std::cerr << "Hello World!" << std::endl;
  return 0;
}

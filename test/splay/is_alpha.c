// RUN: %clang -O0 -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2
// XFAIL: *
// This fails because of unsupported static data.

#include <ctype.h>

int main(void) {
  return isalpha('1');
}

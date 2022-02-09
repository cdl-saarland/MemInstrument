// RUN: %clang -O0 -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -lm -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <math.h>

int main(void) {
  return atan(1.0) > 3.0;
}

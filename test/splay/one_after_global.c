// RUN: %clang -emit-llvm -Xclang -disable-O0-optnone -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument -mi-config=splay %t0.ll -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 2> %t3.log
// RUN: %not fgrep "Memory safety violation!" %t3.log
// XFAIL: *
// This requires handling of pointers one after an object
#include <stdlib.h>

#define num 8
int A[num];
int B[num];

int sum(int *end, int n) {
  int *it = end;
  int res = 0;
  while (n --> 0) {
    res += *--end;
  }
  return res;
}

void __splay_check_dereference(void*, void*, size_t);

int main(void) {
  for (int i = 0; i < num; ++i) {
    B[i] = i;
  }
  sum(&B[num], num);

  return 0;
}



// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdarg.h>
#include <stdio.h>

typedef struct dummy { int i; int j; } dummyS;
typedef struct byVal { double k; int l; dummyS* ptr; } byValS;

void by_val_fun(char *fmt, byValS byValueStruct) {
  printf("First: %c Values: {%f, %d, %p}\n", *fmt, byValueStruct.k, byValueStruct.l, byValueStruct.ptr);
}

int main() {
  dummyS du = {36, 'a'};
  byValS badStruct = {101.1, 102, &du};

  by_val_fun("1", badStruct);

  return 0;
}

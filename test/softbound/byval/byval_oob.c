// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %not --crash %t

#include <stdarg.h>
#include <stdio.h>

typedef struct some { int i; int j; } someS;
typedef struct byVal { double k; int l; someS* ptr; } byValS;

void by_val_fun(char *fmt, byValS byValueStruct) {
  printf("First: %c Values: {%f, %d, %p}\n", *fmt, byValueStruct.k, byValueStruct.l, *(&byValueStruct.ptr + 1));
}

int main() {
  someS du = {36, 'a'};
  byValS badStruct = {101.1, 102, &du};

  by_val_fun("1", badStruct);

  return 0;
}

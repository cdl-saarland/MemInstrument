// RUN: export MI_CONFIG=splay; %clang -O1 -Xclang -load -Xclang %passlib -emit-llvm -c -S -o %t1.ll %s
// RUN: fgrep "splay" %t1.ll

#include <stdlib.h>

int main(void)
{
  int *a = malloc(10*sizeof(int));
  return a[5];
}


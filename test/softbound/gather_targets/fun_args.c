// RUN: %clang -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=gatheritargets -mllvm -debug-only=meminstrument-itargetprovider -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: dereference check with constant size 8B for q at entry
// CHECK-NEXT: dereference check with constant size 4B{{.*}}at entry
// CHECK-NEXT: dereference check with constant size 4B for p at entry

// CHECK: value invariant for a at entry::[store,{{.*}}]
// CHECK-NEXT: dereference check with constant size 8B for b at entry::[store,{{.*}}]
// CHECK-NEXT: call invariant for f with 2 arg(s) at entry::call
// CHECK-NEXT: call invariant for softboundcets_printf with 1 arg(s) at entry::call1

// REQUIRES: asserts

#include <stdio.h>

int a;

int f(int *p, int x, int **q) {
  x += **q;
  return *p + x;
}

int main() {
  int *b = &a;
  int res = f(&a, 5, &b);
  printf("%i\n", res);
  return 0;
}

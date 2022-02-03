// RUN: %clang -O1 %s -S -emit-llvm -o %t
// RUN: %opt %loadlibs -meminstrument -S -mi-config=softbound -mi-no-obfuscated-ptr-load-store-transformation=0 -stats %t 2>&1 | %filecheck %s

// CHECK-NOT: Number of integer loads that actually load pointers
// CHECK-NOT: Number of integer stores that actually store pointers

#include <stdio.h>

long x;
long *p;

extern int f(long *, long *);

int main(void) {
  p = &x;
  f(p, &x);
  printf("%ld\n", *p);

  p = &x;
  printf("%ld\n", *p);

  return 0;
}

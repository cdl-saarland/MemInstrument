// RUN: %clang -O1 -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %filecheck %s

// Make sure that nothing is inserted for non-pointer variables
// CHECK: _softboundcets_globals_setup()
// CHECK-NEXT: entry:
// CHECK-NEXT: __softboundcets_init()
// CHECK-NEXT: ret void

// REQUIRES: asserts

#include <stdio.h>
#include <stdlib.h>

int y = 5;
int x = -(10 - 8);

float i = 1.0;

typedef struct {
  int a;
  float b;
} S;

S Ar[2] = {{4, 4.0}, {2, 2.0}};

int main(int argc, char **argv) {
  printf("%i %i %f\n", y, x, i);
  return 0;
}

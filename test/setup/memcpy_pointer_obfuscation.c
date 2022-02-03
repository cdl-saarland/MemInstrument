// RUN: %clang -O1 %s -S -emit-llvm -o %t
// RUN: %opt %loadlibs -meminstrument -S -mi-config=softbound -mi-no-obfuscated-ptr-load-store-transformation=0 -stats %t 2>&1 | %filecheck %s

// CHECK: 1{{.*}}Number of load/store pairs of ints which are actually pointers replaced
// CHECK-DAG: 1{{.*}}Number of metadata loads inserted
// CHECK-DAG: 1{{.*}}Number of metadata stores inserted

// REQUIRES: asserts

#include <string.h>
#include <stdlib.h>

extern int f(int **);

int main(void) {
  int **t = malloc(sizeof(int **));
  int **q = malloc(sizeof(int **));
  f(q);
  f(t);

  memcpy(q, t, sizeof(int *));

  f(q);

  return 0;
}

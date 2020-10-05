// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

int *Ar;

int main(int argc, char **argv) {
  Ar = malloc(10 * sizeof(int));
  printf("%i\n", Ar[8]);
  return 0;
}

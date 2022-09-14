// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

int Ar[10];

int main(int argc, char **argv) {
  printf("%i\n", Ar[8]);
  return 0;
}

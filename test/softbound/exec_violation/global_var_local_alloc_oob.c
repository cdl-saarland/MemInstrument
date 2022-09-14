// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

#include <stdio.h>
#include <stdlib.h>

int *Ar;

int main(int argc, char **argv) {
  Ar = malloc(10 * sizeof(int));
  printf("%i\n", Ar[11]);
  return 0;
}

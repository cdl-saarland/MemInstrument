// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

#include <stdio.h>
#include <stdlib.h>

int Ar[10];

int main(int argc, char **argv) {
  printf("%i\n", Ar[11]);
  return 0;
}

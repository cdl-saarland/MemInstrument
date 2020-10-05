// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <string.h>
#include <stdlib.h>

int main(void) {
  int* t = malloc(sizeof(int) * 20);

  int* s = t + 4;

  memmove(t, s, sizeof(int) * 6);

  return 0;
}

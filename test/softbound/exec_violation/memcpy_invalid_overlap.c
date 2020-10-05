// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

// The non-overlap is currently not checked
// XFAIL: *

#include <string.h>
#include <stdlib.h>

int main(void) {
  int* t = malloc(sizeof(int) * 20);

  int* s = t + 4;

  memcpy(t, s, sizeof(int) * 6);

  return 0;
}

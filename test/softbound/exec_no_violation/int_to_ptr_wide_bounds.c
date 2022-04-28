// RUN: %clang -O1 -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-sb-inttoptr-wide-bounds %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

int x;
int *y = &x;

int f() {
    return (int) y;
}

int main(int argc, char **argv) {
  int val = *((int *)f());
  printf("%i\n", val);
  return 0;
}

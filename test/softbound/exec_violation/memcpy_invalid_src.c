// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

#include <string.h>
#include <stdlib.h>

struct S {
  int a;
  int b;
  struct S* next;
};

int main(void) {
  struct S* t = malloc(2);

  struct S s;

  memcpy(&s, t, sizeof(struct S));

  return 0;
}

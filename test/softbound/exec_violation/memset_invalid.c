// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null


#include <string.h>
#include <stdlib.h>

struct S {
  char a;
  char b;
  char c;
  struct S* next;
};

int main(void) {
  struct S* t = malloc(sizeof(struct S));

  memset(t, 42, sizeof(struct S) + 1);

  return t->b - 42;
}

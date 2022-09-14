// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

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

  memset(t, 42, sizeof(struct S));

  return t->b - 42;
}

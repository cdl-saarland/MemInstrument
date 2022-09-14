// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <string.h>
#include <stdlib.h>

struct S {
  int a;
  int b;
  struct S* next;
};

int main(void) {
  struct S s;
  s.a = 42;
  s.b = 17;
  s.next = &s;

  struct S* t = malloc(sizeof(struct S));

  memcpy(t, &s, sizeof(struct S));

  return t->b - 17;
}
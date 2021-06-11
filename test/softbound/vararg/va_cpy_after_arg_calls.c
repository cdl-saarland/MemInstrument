// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <stdarg.h>

void testVaCopyArg(char *fmt, ...) {
  va_list ap, aq;
  va_start(ap, fmt);

  for (int i = 0; i < 3; i++) {
    char *s = va_arg(ap, char *);
    printf("Arg %d: %c\n", i, *s);
  }

  va_copy(aq, ap);

  for (int i = 3; i < 6; i++) {
    char *s = va_arg(aq, char *);
    printf("Arg %d: %c\n", i, *s);
  }

  for (int i = 3; i < 6; i++) {
    char *s = va_arg(ap, char *);
    printf("Arg %d: %c\n", i, *s);
  }

  va_end(ap);
  va_end(aq);
}

int main() {
  testVaCopyArg("s", "first", "second", "third", "fourth", "fifth", "sixth");
  return 0;
}

// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdarg.h>
#include <stdio.h>

void printall(int num, va_list args) {
  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    printf("Next: %c\n", *(next + 1));
  }
}

void printlist(int num, va_list args) {
  if (num > 0) {
    char *first = va_arg(args, char *);
    printf("First: %c\n", *(first + 1));
    printall(--num, args);
  }
}

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);
  printlist(num, args);
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd", "ef");
    return 0;
}

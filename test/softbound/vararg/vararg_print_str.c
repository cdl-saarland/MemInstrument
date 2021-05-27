// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdarg.h>
#include <stdio.h>

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);

  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    printf("%s\n", next + 1);
  }
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd", "ef");
    return 0;
}

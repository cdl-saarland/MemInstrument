// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdarg.h>
#include <stdio.h>

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);

  for (int i = 0; i < num; i++) {
    if (i % 2 == 0) {
        double next = va_arg(args, double);
        printf("%f\n", next);
    } else {
        int next = va_arg(args, int);
        printf("%d\n", next);
    }
  }
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, 1.0, 2, 3.0);
    return 0;
}

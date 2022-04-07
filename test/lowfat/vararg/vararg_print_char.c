// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linklowfat -o %t
// RUN: %t

#include <stdio.h>

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);

  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    int loaded = *next;
    printf("%c\n", loaded);
  }
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd", "ef");
    return 0;
}

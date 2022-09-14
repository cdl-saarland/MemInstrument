// RUN: %not %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S 2>&1 | %filecheck %s

// Varargs are currently only supported for the common case that the va_list
// which stores them is a simple stack allocation
// CHECK: Meminstrument Error

#include <stdio.h>
#include <stdlib.h>

void printargs(int num, ...) {
  va_list *args = malloc(sizeof(va_list));
  va_start(*args, num);

  for (int i = 0; i < num; i++) {
    char *next = va_arg(*args, char *);
    int loaded = *next;
    printf("%c\n", loaded);
  }
  va_end(*args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd", "ef");
    return 0;
}

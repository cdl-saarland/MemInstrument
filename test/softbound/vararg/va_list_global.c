// RUN: %not %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S 2>&1 | %filecheck %s

// Varargs are currently only supported for the common case that the va_list
// which stores them is a simple stack allocation
// CHECK: Meminstrument Error

#include <stdio.h>

va_list gl_args;

void printSingle(int num) {

    for (int i = 0; i < num; i++) {
        char *next = va_arg(gl_args, char *);
        int loaded = *next;
        printf("%c\n", loaded);
    }
}

void printArgs(int num, ...) {
    va_list args;
    va_start(args, num);
    va_copy(gl_args, args);

    printSingle(num);

    va_end(gl_args);
    va_end(args);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

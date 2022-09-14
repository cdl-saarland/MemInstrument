// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdio.h>

void printList(int num, va_list args) {
    printf("%d\n", num);
}

void printArgs(int num, ...) {
    va_list args;
    va_start(args, num);
    printList(num, args);
    va_end(args);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

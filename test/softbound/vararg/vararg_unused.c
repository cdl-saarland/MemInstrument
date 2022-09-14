// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdio.h>

void printArgs(int num, ...) {
    printf("%d\n", num);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

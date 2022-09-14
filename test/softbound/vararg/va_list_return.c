// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

va_list *reallyPrint(int num, va_list *args) {

    for (int i = 0; i < num - 1; i++) {
        char *next = va_arg(*args, char *);
        int loaded = *next;
        printf("%c\n", loaded);
    }

    return args;
}

void printArgs(int num, ...) {
    va_list args;
    va_start(args, num);
    va_list *retargs = reallyPrint(num, &args);
    char *next = va_arg(*retargs, char *);
    printf("%c\n", *next);
    va_end(*retargs);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

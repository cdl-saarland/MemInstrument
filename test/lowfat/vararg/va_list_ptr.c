// RUN: %clang -mcmodel=large -fplugin=%passlib -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linklowfat -o %t
// RUN: %t

#include <stdio.h>

void printSingleNoPtr(int num, va_list args) {

    for (int i = 0; i < num; i++) {
        char *next = va_arg(args, char *);
        int loaded = *next;
        printf("%c\n", loaded);
    }
}

void printSingle(int num, va_list *args) {

    for (int i = 0; i < num; i++) {
        char *next = va_arg(*args, char *);
        int loaded = *next;
        printf("%c\n", loaded);
    }
}

void printArgs(int num, ...) {
    va_list args;
    va_list copied;
    va_start(args, num);
    va_copy(copied, args);

    printSingle(num, &args);
    printSingleNoPtr(num, copied);

    va_end(args);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

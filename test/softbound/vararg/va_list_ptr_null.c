// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %linksb %t0.ll -o %t
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

    if (args) {
        for (int i = 0; i < num; i++) {
            char *next = va_arg(*args, char *);
            int loaded = *next;
            printf("%c\n", loaded);
        }
    }
}

void printArgs(int num, ...) {
    printSingle(num, NULL);

    va_list args;
    va_start(args, num);

    printSingleNoPtr(num, args);

    va_end(args);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

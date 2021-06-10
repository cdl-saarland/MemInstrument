// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %linksb %t0.ll -o %t
// RUN: %t

#include <stdio.h>

void printStuff(int num, va_list args, va_list moreArgs) {

    for (int i = 0; i < num; i++) {
        char *next = va_arg(args, char *);
        char *other = va_arg(moreArgs, char *);
        printf("%c %c\n", *next, *other);
    }
}

void printArgs(int num, ...) {
    va_list args;
    va_list copied;
    va_start(args, num);
    va_copy(copied, args);

    char *next = va_arg(args, char *);
    printf("%c\n", *next);

    printStuff(--num, args, copied);

    va_end(copied);
    va_end(args);
}

int main(int argc, char const *argv[]) {
    printArgs(3, "ab", "cd", "ef");
    return 0;
}

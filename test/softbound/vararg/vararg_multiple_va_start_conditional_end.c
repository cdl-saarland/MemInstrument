// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %linksb %t0.ll -o %t
// RUN: %t

#include <stdio.h>
#include <stdarg.h>

void testMultiConditionalStart(int x, int y, ...) {
    va_list args;

    if (x > 10) {
        va_start(args, y);
        for (int i = 0; i < 3; i++) {
            char *next = va_arg(args, char *);
            printf("Next: %c\n", *next);
        }
    }

    if (y < 5) {
        va_start(args, y);
        for (int i = 0; i < 3; i++) {
            char *next = va_arg(args, char *);
            printf("Next: %c\n", *(next+ y));
        }
    }

    if (x > 10 || y < 5) {
        va_end(args);
    }
}

int main() {
    testMultiConditionalStart(7, 1, "first", "second", "third", "fourth", "fifth", "sixth");
    return 0;
}

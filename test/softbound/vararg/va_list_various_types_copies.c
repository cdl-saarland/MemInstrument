// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %linksb %t0.ll -o %t
// RUN: %t

#include <stdarg.h>
#include <stdio.h>

void printNulTermStr(char *str) {
    while (*str != '\0') {
        printf("%c", *str);
        str++;
    }
    printf(" ");
}

char *printFloats(char *beginText, va_list args, int num) {
    printNulTermStr(beginText);
    printf("\n");

    for (int i = 0; i < num; i++) {
        double toPrint = va_arg(args, double);
        printf("%f\n", toPrint);
    }

    return "Finished float printing.\n";
}

void printList(int num, va_list args) {
    for (int i = 0; i < num; i++) {
        char *next = va_arg(args, char *);
        printNulTermStr(next);
    }
    printf("\n");
}


void printargs(char *begin, int x, float y, char *moreText, int z, ...) {
    va_list args;
    va_list copied;
    va_start(args, z);

    printNulTermStr(begin);
    printNulTermStr(moreText);

    printList(2, args);

    va_copy(copied, args);
    char *finishedText = printFloats("Start float printing...", copied, 2);
    printNulTermStr(finishedText);
    printList(1, copied);
    va_end(copied);

    finishedText = printFloats("Start float printing 2...", args, 2);
    printNulTermStr(finishedText);
    printList(1, args);

    va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs("test", 5, 3.0, "other", 3, "ab", "cd", 5.0, 4.0, "ef");
    return 0;
}

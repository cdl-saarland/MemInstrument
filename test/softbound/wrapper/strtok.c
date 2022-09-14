// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t "hallo::world"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

int printByteWise(char *str) {

    while (*str != '\0') {
        printf("%c", *str);
        str++;
    }
    printf("\n");
    return *str;
}

int main(int argc, char *argv[]) {

    char *subStr = strtok(argv[1], ":");
    while (subStr) {
        printByteWise(subStr);
        subStr = strtok(NULL, ":");
    }

    return 0;
}

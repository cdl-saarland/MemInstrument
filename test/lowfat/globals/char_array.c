// RUN: %clang -fplugin=%passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t 1 1 1 1 1 1 1 1 1 1 1 1 1

// Test the corner case that the last valid index of the global array (14) is
// accessed.

#include <stdio.h>

char Ar[15];

int main(int argc, char const *argv[]) {
    printf("Num args: %d\n", argc);
    printf("%c\n", Ar[argc]);
    return 0;
}

// RUN: %clang -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %not --crash %t 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2>&1 | %filecheck %s

// CHECK: Out-of-bounds pointer dereference!

// The global has size 15, one is added such that one-past-the-end pointers
// won't cause errors, and 16 already is a lf size. Hence, accessing location
// index 16 should cause an error.

#include <stdio.h>

char Ar[15];

int main(int argc, char const *argv[]) {
    printf("Num args: %d\n", argc);
    printf("Entry there: %c\n", Ar[argc]);
    return 0;
}

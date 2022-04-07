// RUN: %clang -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t

#include <stdio.h>

int a;

int main(int argc, char const *argv[]) {
    printf("%d\n", a);
    return 0;
}

// RUN: %clang -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t

#include <stdio.h>

char a;

int main(int argc, char const *argv[]) {
    a = argc > 5? argv[1] : 'z';
    printf("%c\n", a);
    return 0;
}

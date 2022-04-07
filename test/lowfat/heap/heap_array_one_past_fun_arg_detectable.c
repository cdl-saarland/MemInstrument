// RUN: %clang -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %not --crash %t 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2>&1 | %filecheck %s

// CHECK: Out-of-bounds pointer dereference!

#include <stdio.h>
#include <stdlib.h>

void do_print(char *p) {
    printf("%c", *p);
}

int main(int argc, char const *argv[]) {
    char *Ar = malloc(15);
    printf("Num args: %d\n", argc);
    do_print(Ar + argc);
    return 0;
}

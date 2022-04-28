// RUN: %clang -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %not --crash %t 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2>&1 | %filecheck %s

// CHECK: Out-of-bounds pointer dereference!

#include <stdio.h>

char Ar[15];

void do_print(char *p) {
    printf("%d", *((int *)p));
}

int main(int argc, char const *argv[]) {
    printf("Num args: %d\n", argc);
    do_print(Ar + argc);
    return 0;
}

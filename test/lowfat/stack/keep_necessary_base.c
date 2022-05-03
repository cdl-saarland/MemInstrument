// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -mllvm -mi-lf-calculate-base-lazy=0 -o %t.ll -emit-llvm -S
// RUN: %filecheck %s < %t.ll
// CHECK: __lowfat_ptr_base_without_index

#include <stdio.h>
#include <stdlib.h>

void foo(int x);

int main(int argc, char const *argv[]) {
    int * Ar = malloc(15 * sizeof(*Ar));

    Ar[0] = 2;
    Ar[1] = 3;
    Ar[2] = 4;

    foo(Ar[argc]);
    return 0;
}


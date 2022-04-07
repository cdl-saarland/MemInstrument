// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -o %t
// RUN: %t 1 2 3

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    int Ar[argc];
    printf("Hello %p\n", Ar);

    Ar[2] = 1;

    printf("Test\n");

    printf("%p %i\n", Ar, Ar[argc-1]);
    return 0;
}

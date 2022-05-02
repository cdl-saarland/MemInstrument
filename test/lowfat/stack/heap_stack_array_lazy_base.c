// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat -mllvm -mi-lf-calculate-base-lazy %linklowfat -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

int x;

int *f(int *p) {
    p[x] = 0;
    return p+5;
}

int main(int argc, char const *argv[]) {
    int Ar[15];
    x = 2;

    int *q = f(Ar);
    int *m = malloc(5 * sizeof(int));
    int *l = f(m);

    printf("%p %i %p\n", q, q[2], l);
    return 0;
}

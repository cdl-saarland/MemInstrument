// RUN: %clang -mcmodel=large -fplugin=%passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    char *Ar = malloc(15);
    printf("Hello %p\n", Ar);

    Ar[4] = 1;

    printf("Test\n");

    printf("%p %i\n", Ar, Ar[argc-1]);
    return 0;
}

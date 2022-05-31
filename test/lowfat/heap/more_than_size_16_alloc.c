// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -o %t
// RUN: %t

#include <stdlib.h>
#include <stdio.h>

void print_ptr(int *ptr) {
    printf("%p\n", ptr);
}

int main(int argc, char const *argv[]) {
    int *p = (int *)malloc(100 * sizeof(int));
    print_ptr(p);
    return 0;
}

// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -o %t
// RUN: %t

// TODO Use filecheck on rt stats (see std_err_unchecked)

#include <stdio.h>

int x;
int *gl_ptr = &x;

void print_ptr(int *ptr) {
    fprintf(stderr, "%p\n", ptr);
}

int main(int argc, char const *argv[]) {
    print_ptr(gl_ptr);
    return 0;
}

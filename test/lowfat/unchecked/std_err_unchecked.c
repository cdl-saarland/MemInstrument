// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 -mllvm -mi-config=lowfat %linklowfat -o %t -mllvm -stats %s 2>&1 | %filecheck %s
// RUN: %t

// CHECK: 1{{.*}}dereference checks inserted
// TODO It would be nice to check the runtime statistics which distinguish fat/non-fat pointer checks. This requires a feature test for enabled runtime statistics.

// REQUIRES: asserts

#include <stdlib.h>
#include <stdio.h>

void print_ptr(int *ptr) {
    fprintf(stderr, "%p\n", ptr);
}

int main(int argc, char const *argv[]) {
    int *p = (int *)malloc(400 * sizeof(int));
    print_ptr(p);
    return 0;
}

// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linkltosb -o %t
// RUN: %t 2 2 2 2 2 2 2 2 2 2 2 2 2 2

// REQUIRES: lto

#include <stdio.h>

int main(int argc, char const *argv[]) {

    int Ar[15];

    for (int i = 0; i < argc; i++) {
        Ar[i] = 1;
        printf("Ar[%i]: %i\n", i, Ar[i]);
    }

    return 0;
}

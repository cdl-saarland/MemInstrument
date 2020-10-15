// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

// This test case should report an error as the char array `some` is not null
// terminated, so it should better not be handed over to strcpy.
// XFAIL: *

#include <stdio.h>
#include <string.h>

int main(void) {
    char some[5];
    memset(some, 'a', 5);
    char other[10];
    strcpy(other, some);
    printf("%s\n", other);
    return 5;
}

// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2> /dev/null

#include <stdio.h>

int main(int argc, char const *argv[]) {

    int Ar[15];

    // Use argc instead of a constant upper bound to avoid that the overflow is
    // statically detectable.
    for (int i = 0; i < argc; i++) {
        Ar[i] = 1;
        printf("Ar[%i]: %i\n", i, Ar[i]);
    }

    return 0;
}

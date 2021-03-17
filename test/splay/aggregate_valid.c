// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=splay %linksplay -o %t
// RUN: %t

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *x;
    int *ptr;
} InfoWithPtr;


InfoWithPtr f();

int main(int argc, char const *argv[]) {
    InfoWithPtr thing = f();
    thing.x = malloc(20 * sizeof(int));
    printf("%i %i\n", thing.ptr[0], thing.x[15]);
    return 0;
}

InfoWithPtr f() {
    InfoWithPtr thing;
    thing.ptr = malloc(1 * sizeof(int));
    return thing;
}

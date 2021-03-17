// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1  %s -mllvm -mi-config=softbound -mllvm -mi-mode=gatheritargets -mllvm -debug-only=meminstrument-itargetprovider -emit-llvm -o - 2>&1 | %fileCheck %s


// Function main

// Function f
// CHECK: value invariant {{.*}}insert
// CHECK: value invariant {{.*}}ret

// REQUIRES: asserts

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x;
    int *ptr;
} InfoWithPtr;


InfoWithPtr f();

int main(int argc, char const *argv[]) {
    InfoWithPtr thing = f();

    printf("%p %i\n", thing.ptr, thing.x);
    return 0;
}

InfoWithPtr f() {
    InfoWithPtr thing;
    thing.x = 5;
    thing.ptr = malloc(1);
    return thing;
}

// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1  %s -mllvm -mi-config=softbound -mllvm -mi-mode=genchecks -mllvm -debug-only=softbound-genchecks -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: argument 0 invariant
// CHECK-NEXT: Passed pointer information stored to shadow stack.
// CHECK: argument 1 invariant
// CHECK-NEXT: Passed pointer information stored to shadow stack.
// CHECK: value invariant at {{.*}}insert
// CHECK-NEXT: Nothing to do.
// CHECK: value invariant for {{.*}}insert at {{.*}}ret
// CHECK-NEXT: Returned pointer information stored to shadow stack.

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

// RUN: %clang -Xclang -load -Xclang %passlib -O3 %s -mllvm -mi-config=softbound -emit-llvm -S -o - 2>&1 | %filecheck %s

// CHECK: load i32, i32* getelementptr inbounds (%struct.a_simple_pair, %struct.a_simple_pair* @P, i64 0, i32 0)

// This test shows that the narrowing of bounds on IR level is basically an
// illusion. The compiler knows the layout of structs, and makes use of this
// information to transform code such as the one below to a direct accesses of a
// field (in this case x).

#include <stdio.h>

struct a_simple_pair {
    int x;
    int y;
} P;

void do_stuff(int *some_ptr) {
    printf("The value is %d\n", *(some_ptr - 1));
}

int main() {
    P.x = 2;
    P.y = 3;
    do_stuff(&P.y);
    return 0;
}

// RUN: %clang -O3 %s -emit-llvm -S -o - 2>&1 | %filecheck %s

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

void print_value(int *some_ptr) {
    printf("The value is %d\n", *some_ptr);
}

int main() {
    print_value(&P.y - 1);
    return 0;
}

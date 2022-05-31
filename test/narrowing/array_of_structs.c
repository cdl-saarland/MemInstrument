// RUN: %clang -O1 %s -S -c -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: define{{.*}}void @do_stuff(%struct.a_simple_pair*{{.*}})

#include <stdio.h>

struct a_simple_pair {
    int x;
    int y;
};

typedef struct a_simple_pair a_simple_pair;

a_simple_pair pair_array[10];

void do_stuff(a_simple_pair *p) {
    for (int i = 0; i < 5; i++) {
        printf("x: %i\ny: %i\n", p->x, p->y);
        p++;
    }
}

int main() {
    for (int i = 0; i < 10; i++) {
        pair_array[i].x = i;
        pair_array[i].y = 10 - i;
    }
    do_stuff(&pair_array[5]);
    return 0;
}

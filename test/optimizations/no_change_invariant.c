// RUN: %clang -O0 -Xclang -disable-O0-optnone %s -c -S -emit-llvm -o %t
// RUN: %opt -S -mem2reg -load %passlib -meminstrument -mi-config=splay -stats %t 2>&1 | %filecheck %s

// CHECK: 1 {{.*}} inbounds targets discarded because of witness graph information
// CHECK: 2 {{.*}} dereference checks inserted

int **p;

int *example_no_change() {
    int *q = *p;
    // Invariant check on q can be skipped as we loaded it as is from memory
    return q;
}

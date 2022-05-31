// RUN: %clang -S -c -emit-llvm -O0 -Xclang -disable-O0-optnone %s -o %t.ll
// RUN: %opt -mem2reg -elim-avail-extern %loadlibs -meminstrument -mi-config=softbound -S %t.ll -stats -o %t.instrumented.ll 2>&1 | %filecheck %s

// CHECK: 2 softbound{{.*}}metadata loads inserted
// CHECK: 2 softbound{{.*}}metadata stores inserted

// REQUIRES: asserts

void swap(double **first, double **second) {
    double *tmp = *first;
    *first = *second;
    *second = tmp;
}

// RUN: %clang -O1 %s -c -S -emit-llvm -o %t.ll
// RUN: %opt -S -mem2reg -load %passlib -meminstrument -mi-config=splay -mi-opt-dominance -mi-opt-dominance-optimize-invariants -stats %t.ll 2>&1 | %filecheck %s

// CHECK: 1 {{.*}} instrumentation targets discarded because of dominating subsumption

// REQUIRES: asserts

int **p;
int x;

int do_stuff(int *q);

void example_invar_dom_opt() {
    x = *(*p + 5);
    // Invariant does not need to be checked as a dominating location already
    // checked the pointer
    do_stuff(*p + 5);
}

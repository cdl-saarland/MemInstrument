// RUN: %clang -O0 -Xclang -disable-O0-optnone %s -c -S -emit-llvm -o %t
// RUN: %opt -S -mem2reg -load %passlib -meminstrument -mi-config=splay -mi-opt-dominance -stats %t 2>&1 | %filecheck %s

// CHECK: 1 {{.*}} discarded because of dominating subsumption
// CHECK: 1 {{.*}} dereference checks inserted


int example_dom_opt(int *q) {
    if (*q > 0) {
        // Dereference check on q can be skipped as a dominating location
        // already checked it
        return *q;
    }
    return 0;
}

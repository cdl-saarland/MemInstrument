// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2
// XFAIL: *
// We do not support pvalloc

#include <stdlib.h>
#include <malloc.h>

int main(void)
{
    void *e = pvalloc(16);

    free(e);
    return 0;
}


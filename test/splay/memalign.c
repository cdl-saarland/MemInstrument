// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <stdlib.h>
#include <malloc.h>

int main(void)
{
    void *a = NULL;
    posix_memalign(&a, 16, 64);
    void *b = memalign(16, 64);
    void *c = aligned_alloc(16, 64);
    void *d = valloc(16);
    /* void *e = pvalloc(16); */

    free(a);
    free(b);
    free(c);
    free(d);
    /* free(e); */
    return 0;
}


// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -mi-verbose -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <stdlib.h>
#include <sys/mman.h>

int main(void)
{
    int *p = (int*) mmap(NULL, 10 * sizeof(int), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    for (int i = 0; i < 10; ++i) {
        p[i] = i;
    }
    int res = p[3];
    munmap(p, 10 * sizeof(int));

    return res - 3;
}


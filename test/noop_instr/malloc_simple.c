// RUN: %clang -emit-llvm -O0 -Xclang -disable-O0-optnone -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=noop -mi-noop-time-deref-check=10 -S > %t1.ll
// RUN: egrep "call .* @usleep" %t1.ll
// RUN: %clink -o %t2 %t1.ll
// RUN: %t2

#include <stdlib.h>

int main(void)
{
    int * p = malloc(4*sizeof(int));
    p[0] = 2;
    p[1] = 3;
    return 0;
}

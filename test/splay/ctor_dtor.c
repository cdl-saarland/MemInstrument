// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 > %t3.log
// RUN: fgrep "CTOR" %t3.log
// RUN: fgrep "DTOR" %t3.log

#include <stdio.h>

void ctor() __attribute__((constructor));

void ctor() {
   printf("CTOR\n");
}
void dtor() __attribute__((destructor));

void dtor() {
   printf("DTOR!\n");
}

int main() { return 0; }

// RUN: %clang -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1

#include <stdio.h>

int a;

struct S {
    int x;
    int *aPtr;
    int Ar[20];
    char b;
} myS;

int main(int argc, char const *argv[]) {
    myS.aPtr = &a;
    myS.x = 2;
    myS.b = 'a';
    *myS.aPtr = 5;
    myS.Ar[argc] = a;
    myS.Ar[7] = 1;
    printf("%d %d %d %c %d\n", a, myS.x, *myS.aPtr, myS.b, myS.Ar[19]);
    return 0;
}

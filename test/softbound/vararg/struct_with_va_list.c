// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

struct containsVaList {
    int *ptr;
    int a;
    va_list *theList;
    int *q;
};

typedef struct containsVaList containsVaList;

int myGlobal;
int myOtherGlobal;

int g(containsVaList *sWithVaList) {

    char *s = va_arg(*sWithVaList->theList, char *);
    printf("%s\n", s);

    s = va_arg(*sWithVaList->theList, char *);
    printf("%c\n", *s);

    return *sWithVaList->ptr;
}

int f(int num, ...) {

    containsVaList structWithVaList;

    va_list args;
    va_start(args, num);

    structWithVaList.ptr = &myGlobal;
    structWithVaList.a = myGlobal;
    structWithVaList.theList = &args;
    structWithVaList.q = &myOtherGlobal;

    int res = g(&structWithVaList);

    myGlobal = 5;

    va_end(args);
    return res;
}

int main(int argc, char const *argv[]) {
    return f(3, "a", "bc", "def");
}

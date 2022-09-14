// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>

int a = 5;
int *p;

int *stuff() {
    return p;
}

int *printInt(int *x) {
    printf("Some int: %i\n", *x);
    return x;
}

int main() {
    p = &a;
    int *p_get = stuff();
    int *p_later = printInt(p_get);
    printf("Later: %i\n", *p_later);
    return 0;
}

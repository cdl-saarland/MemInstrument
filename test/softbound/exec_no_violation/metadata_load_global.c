// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>

int a = 5;
int *p;

int *get() {
    return p;
}

int main() {
    p = &a;
    int *p_get = get();
    printf("Stored value: %i\n", *p_get);
    return 0;
}

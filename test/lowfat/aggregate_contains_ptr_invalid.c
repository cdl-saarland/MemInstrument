// RUN: %clang -mcmodel=large -fplugin=%passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -o %t
// RUN: %not --crash %t 2> /dev/null

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x;
    int *ptr;
} InfoWithPtr;


InfoWithPtr f();

int main(int argc, char const *argv[]) {
    InfoWithPtr thing = f();

    printf("%i %i\n", *(thing.ptr+17), thing.x);
    return 0;
}

InfoWithPtr f() {
    InfoWithPtr thing;
    thing.x = 5;
    thing.ptr = malloc(1);
    return thing;
}

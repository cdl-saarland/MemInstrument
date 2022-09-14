// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <ctype.h>

int main(int argc, char const *argv[]) {
    int v = 32;
    printf("Is %i a space? %i\n", v, isspace(v) != 0);
    v = 65;
    printf("Is %i a space? %i\n", v, isspace(v) != 0);
    return 0;
}

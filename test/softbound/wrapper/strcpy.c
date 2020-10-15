// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <string.h>

int main(void) {
    char *some = "some";
    char other[25];
    strcpy(other, some);
    printf("%s\n", other);
    return 0;
}

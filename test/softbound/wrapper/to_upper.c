// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <ctype.h>

int main(int argc, char const *argv[]) {
    char v = 'a';
    printf("%c as upper: %c\n", v, toupper(v));
    return 0;
}

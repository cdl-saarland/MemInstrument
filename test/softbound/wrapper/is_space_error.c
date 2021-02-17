// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

#include <stdio.h>
#include <ctype.h>

int main(int argc, char const *argv[]) {
    int v = 5000;
    printf("Is %i a space? %i\n", v, isspace(v) != 0);
    return 0;
}

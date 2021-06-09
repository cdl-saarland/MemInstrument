// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <string.h>
#include <stdio.h>

int foo(const char *x, const char *y, int len) {
    return memcmp(x, y, len);
}

int main() {
    printf("%d\n", foo("perlio", "unix", 3));
    return 0;
}

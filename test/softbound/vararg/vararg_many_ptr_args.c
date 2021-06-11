// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -o %t0.ll
// RUN: %clang %t0.ll %linksb -o %t
// RUN: %t

#include <stdio.h>

void printStuff(char *a, char *b, char *c, char *d, int num, ...) {
    va_list args;

    va_start(args, num);

    // Print the first four strings
    // Note that we use %c rather than %s to have a dereference to check
    // available in this function
    char *next = a;
    while (*next != '\0') {
        printf("%c", *next);
        next++;
    }
    printf(" ");

    next = b;
    while (*next != '\0') {
        printf("%c", *next);
        next++;
    }
    printf(" ");

    next = c;
    while (*next != '\0') {
        printf("%c", *next);
        next++;
    }
    printf(" ");

    next = d;
    while (*next != '\0') {
        printf("%c", *next);
        next++;
    }
    printf(" ");

    // Print all vaargs
    for (int i = 0; i < num; i++) {
        char *read = va_arg(args, char *);
        while (*read != '\0') {
            printf("%c", *read);
            read++;
        }
        printf(" ");
    }
    printf("\n");
    va_end(args);

}

int main(int argc, char const *argv[]) {
    printStuff("This", "is", "some", "text", 3, "bla", "bla", "bla");
    return 0;
}

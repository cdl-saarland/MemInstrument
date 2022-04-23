// RUN: %clang -mcmodel=large -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=lowfat %linklowfat -o %t
// RUN: %t

// Checks whether accesses to the environment work after modifying it using the
// C library functions provided for this purpose. Note that the purpose of
// printing the environment is to access all variables, which ensures that the
// wrapper for the library functions updated the environment correctly.

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

extern char **environ;

char *test = "test=somelongerval";

void print_env() {
    char *const *printEnvp = environ;
    puts("Environment entires:\n");
    while (*printEnvp != NULL) {
        printf("%p\n", *printEnvp);
        printf("%s\n", *printEnvp);
        printf("size: %lu\n\n", strlen(*printEnvp));
        printEnvp++;
    }
}

void set_env() {
    setenv("test", "testval", 1);
}

void put_env() {
    putenv(test);
}

void unset_env() {
    unsetenv("test");
}

int main(int argc, char const *argv[]) {
    printf("Initial environment:\n");
    print_env();

    set_env();
    printf("\nAfter setenv:\n");
    print_env();

    put_env();
    printf("\nAfter putenv:\n");
    print_env();

    unset_env();
    printf("\nAfter unsetenv:\n");
    print_env();
    return 0;
}

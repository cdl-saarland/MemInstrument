// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

#include <unistd.h>
#include <string.h>
#include <stdio.h>

extern char **environ;

void print_env() {
    char *const *printEnvp = environ;
    puts("Environment entires:\n");
    while (*printEnvp != NULL) {
        printf("%p\n", *printEnvp);
        printf("%s\n", *printEnvp);
        printf("size: %lu\n\n", strlen(*printEnvp));
        printEnvp++;
    }

    // Out-of-bounds access
    printf("%p\n", *(++printEnvp));
}

int main(int argc, char const *argv[]) {
    print_env();
    return 0;
}

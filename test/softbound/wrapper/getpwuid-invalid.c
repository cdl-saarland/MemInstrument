// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -emit-llvm -S -c -o %t.ll
// RUN: %clang %t.ll %linksb -o %t
// RUN: %not --crash %t

#include <pwd.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char const *argv[]) {

    uid_t uid = getuid();
    struct passwd *pwent = getpwuid(uid);
    if (pwent) {
        char *home_dir = pwent->pw_dir;
        printf("Home dir: ");
        while (*home_dir) {
            printf("%c", *(home_dir+2));
            fflush(stdout);
            home_dir++;
        }
        printf("\n");
    }

    return 0;
}

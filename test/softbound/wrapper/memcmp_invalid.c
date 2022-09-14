// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null
// Test that resembles a known error in perlbench.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55309

#include <string.h>
#include <stdio.h>

int foo(const char *x, const char *y, int len) {
    return memcmp(x, y, len);
}

int main() {
    printf("%d\n", foo("perlio", "unix", 6));
    return 0;
}

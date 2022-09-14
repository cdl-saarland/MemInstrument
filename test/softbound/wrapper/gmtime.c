// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdio.h>
#include <time.h>

int main(void) {
    long someNum = 2;
    struct tm *sinceTime = gmtime(&someNum);
    printf("Some infos: %d %d\n", sinceTime->tm_sec, sinceTime->tm_isdst);
    return 0;
}

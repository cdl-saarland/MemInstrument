// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: touch %t1
// RUN: %not --crash %t %t1 2> /dev/null

#include <stdio.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>

int main(int argc, char const *argv[]) {

    if (argc < 2) {
        return 0;
    }

    struct stat buf;
    stat(argv[1], &buf);
    printf("Time last accessed: %s", ctime(&buf.st_atime));
    printf("Time last changed: %s\n", ctime(&buf.st_mtime));

    // Capture the currently accessed and modified times
    struct utimbuf old_time;
    old_time.actime = buf.st_atime;
    old_time.modtime = buf.st_mtime;

    // Set to the times to the current time
    utime(argv[1], NULL);
    stat(argv[1], &buf);
    printf("Time last accessed: %s", ctime(&buf.st_atime));
    printf("Time last changed: %s\n", ctime(&buf.st_mtime));

    // Hand over an invalid address for the utimbuf argument
    utime(argv[1], &old_time + 1);
    stat(argv[1], &buf);
    printf("Time last accessed: %s", ctime(&buf.st_atime));
    printf("Time last changed: %s\n", ctime(&buf.st_mtime));

    return 0;
}

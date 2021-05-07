// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: touch %t1
// RUN: %t %t1

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

    // Restore the previous times
    utime(argv[1], &old_time);
    stat(argv[1], &buf);
    printf("Time last accessed: %s", ctime(&buf.st_atime));
    printf("Time last changed: %s\n", ctime(&buf.st_mtime));

    return 0;
}

// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t %testfolder/softbound/wrapper/somefile.txt

// Source (modified): https://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]) {

    // Make sure that a file name is given
    assert(argc > 1);
    char *file_name = argv[1];

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen(file_name, "r");
    if (fp == NULL)
        exit(1);
    int x = 0;
    while ((read = getdelim(&line, &len, '\n', fp)) != -1) {
        printf("Retrieved line of length %zu:\n", read);
        // Add some additional dereference to make sure that the metadata for
        // the line pointer was propagated correctly.
        x = *line;
        printf("%s\n", line);
    }
    printf("%i\n", x);
    if (ferror(fp)) {
        /* handle error */
    }
    free(line);
    fclose(fp);
    return 0;
}

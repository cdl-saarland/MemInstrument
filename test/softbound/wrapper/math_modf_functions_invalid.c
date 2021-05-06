// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

#include <stdio.h>
#include <math.h>

int main(int argc, char const *argv[]) {

    double df, di;

    df = modf(3.14, &di + 1);

    printf("Double modf result: %lf %lf\n", di, df);

    return 0;
}

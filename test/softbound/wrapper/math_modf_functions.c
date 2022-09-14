// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -mllvm --debug-only=softbound,meminstrument -o %t
// RUN: %t

// REQUIRES: asserts

#include <stdio.h>
#include <math.h>

int main(int argc, char const *argv[]) {

    double df, di;
    float ff, fi;
    long double ldf, ldi;

    df = modf(3.14, &di);
    ff = modff(3.14, &fi);
    ldf = modfl(3.14, &ldi);

    printf("Double modf result: %lf %lf\n", di, df);
    printf("Float modf result: %f %f\n", fi, ff);
    printf("Long double modf result: %Lf %Lf\n", ldi, ldf);

    return 0;
}

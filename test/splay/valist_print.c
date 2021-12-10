// RUN: %clang -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument -mi-config=splay -stats %t0.ll -S > %t1.ll 2> %t4.stats
// RUN: grep "1 meminstrument.*The # of inbounds checks inserted" %t4.stats
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

// REQUIRES: asserts

#include <stdarg.h>
#include <stdio.h>

void printlist(int num, va_list args) {
  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    printf("%s\n", next + 1);
  }
}

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);
  printlist(num, args);
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd", "ef");
    return 0;
}

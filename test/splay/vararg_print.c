// RUN: %clang -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument -mi-config=splay -stats %t0.ll -S > %t1.ll 2> %t4.stats
// RUN: %not fgrep "The # of inbounds checks inserted" %t4.stats
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <stdarg.h>
#include <stdio.h>

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);

  for (int i = 0; i < num; i++) {
    int next = va_arg(args, int);
    printf("%d\n", next);
  }
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, 1, 2, 3);
    return 0;
}

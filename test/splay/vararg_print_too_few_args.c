// RUN: %clang -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument -mi-config=splay -stats %t0.ll -S > %t1.ll 2> %t4.stats
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not --crash %t2 2> %t3.log
// RUN: fgrep "Memory safety violation!" %t3.log

// There is no mechanism in place to detect that more arguments are accessed
// within the function than arguments handed over at the call site.
// XFAIL: *

#include <stdarg.h>
#include <stdio.h>

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);

  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    printf("%s\n", next + 1);
  }
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd");
    return 0;
}

// RUN: %clang -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument -mi-config=splay -stats %t0.ll -S > %t1.ll 2> %t4.stats
// RUN: grep "2 meminstrument.*The # of inbounds checks inserted" %t4.stats
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not --crash %t2 2> %t3.log
// RUN: fgrep "Memory safety violation!" %t3.log

#include <stdarg.h>
#include <stdio.h>

void printall(int num, va_list args) {
  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    printf("Next: %s\n", next - 1);
  }
}

void printlist(int num, va_list args) {
  if (num > 0) {
    char *first = va_arg(args, char *);
    printf("First: %s\n", first + 1);
    printall(--num, args);
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

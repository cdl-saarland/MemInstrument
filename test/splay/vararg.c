// RUN: %clang -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument -mi-config=splay -stats %t0.ll -S > %t1.ll 2> %t4.stats
// RUN: grep "1 meminstrument.*The # of inbounds checks inserted" %t4.stats
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#include <stdio.h>
#include <stdarg.h>

void testVaCopyArg(char *fmt, ...) {
  va_list ap, aq;
  char *s;
  va_start(ap, fmt);
  va_copy(aq, ap);    /* test va_copy */

  s = va_arg(aq, char *);
  printf("string %s\n", s + 1);
  va_end(ap);
  va_end(aq);
}

int main() {
  testVaCopyArg("s", "abc");
  return 0;
}

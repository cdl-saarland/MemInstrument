// RUN: %clang -emit-llvm -Xclang -disable-O0-optnone -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-verbose -mi-config=splay -debug-only=meminstrument -S > %t1.ll 2> %t2.log
// RUN: %not fgrep "skip function" %t2.log
// XFAIL: *

#include <stdio.h>
#include <stdarg.h>

void testVaCopyArg(char *fmt, ...) {
  va_list ap, aq;
  char *s;
  va_start(ap, fmt);
  va_copy(aq, ap);    /* test va_copy */

  s = va_arg(aq, char *);
  printf("string %s\n", s);
}

int main() {
  testVaCopyArg("s", "abc");
  return 0;
}

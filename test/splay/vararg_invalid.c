// RUN: %clang -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument -mi-config=splay -stats -debug-only=VarArgImpl %t0.ll -S > %t1.ll 2> %t4.stats
// RUN: grep "1 meminstrument.*The # of inbounds checks inserted" %t4.stats
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not --crash %t2 2> %t3.log
// RUN: fgrep "Memory safety violation!" %t3.log

// REQUIRES: asserts

#include <stdio.h>
#include <stdarg.h>

void testVaCopyArg(char *fmt, ...) {
  va_list ap, aq;
  char *s;
  va_start(ap, fmt);
  va_copy(aq, ap);    /* test va_copy */

  s = va_arg(aq, char *);
  printf("string %s\n", s + 5);
  va_end(ap);
  va_end(aq);
}

int main() {
  testVaCopyArg("s", "abc");
  return 0;
}

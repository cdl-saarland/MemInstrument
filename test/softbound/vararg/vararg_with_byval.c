// RUN: %not %clang -O1 -fplugin=%passlib %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -emit-llvm -S -o - 2>&1 | %filecheck %s

// CHECK: Meminstrument Error{{.*}}varargs and byvalue arguments{{.*}}

#include <stdarg.h>
#include <stdio.h>

typedef struct some { int i; int j; } someS;
typedef struct bad { double k; int l; someS* ptr; } badS;

void va_arg_and_by_val(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  badS toRead = va_arg(ap, badS);
  printf("First: %c Values: {%f, %d, %p}\n", *fmt, toRead.k, toRead.l, toRead.ptr);

  va_end(ap);
}

int main() {
  someS du = {36, 'a'};
  badS badStruct = {101.1, 102, &du};

  va_arg_and_by_val("1", badStruct);

  return 0;
}

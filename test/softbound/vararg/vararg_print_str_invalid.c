// RUN: %clang -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %not --crash %t 2> /dev/null

// The printf wrapper does not check incoming pointers for valid bounds.
// Whether this pointer is dereferenced depends on the format specifier (%s vs
// %p for example). Throwing an error in case the pointer is not dereferenced
// is against the philosophy of the instrumentation.
// Printf's format specifiers have to be parsed in order to properly report
// errors for cases such as this one. In addition, parsing format specifiers
// would allow to provide guarantees on a matching number of format specifiers
// and arguments handed over to printf.
// XFAIL: *

#include <stdarg.h>
#include <stdio.h>

void printargs(int num, ...) {
  va_list args;
  va_start(args, num);

  for (int i = 0; i < num; i++) {
    char *next = va_arg(args, char *);
    printf("%s\n", next + 3);
  }
  va_end(args);
}

int main(int argc, char const *argv[]) {
    printargs(3, "ab", "cd", "ef");
    return 0;
}

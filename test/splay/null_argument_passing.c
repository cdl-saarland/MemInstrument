// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

#define NULL ((void*)0)

int foo(int *p) {
  if (p == NULL) {
    return 0;
  } else {
    return *p;
  }
}

int main(void)
{
  return foo(NULL);
}


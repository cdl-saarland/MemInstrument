// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not --crash %t2 2> %t3.err
// RUN: fgrep "Memory safety violation!" %t3.err

#define NULL ((void*)0)

int foo(int x, int y) {
  return x + y;
}

int bar(int f(int, int), int x, int y) {
  return f(x, y);
}

int main(int argc, char** argv)
{
  return bar(NULL, 42, -42);
}

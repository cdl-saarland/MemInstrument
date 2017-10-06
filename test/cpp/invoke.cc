// RUN: %clang -O3 -x c++ -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -instnamer -meminstrument %t0.ll -mi-imechanism=splay -S > %t1.ll
// RUN: %clink -lstdc++ -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2

int foo(int x) {
  if (x == 42) {
    throw 42;
  }
  return x;
}

int main(void) {
  try {
    return foo(42);
  } catch (...) {
    return 0;
  }
}

// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -mi-genchecks %t0.ll -mi-imechanism=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not %t2 2> %t3.log
// RUN: fgrep "Memory safety violation!" %t3.log

int main(void)
{
  int *a = (void*)0;
  return a[5];
}


// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -mi-genchecks %t0.ll -mi-imechanism=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not %t2 foo0 2> %t3.err
// RUN: fgrep "Memory safety violation!" %t3.err

int main(int argc, char** argv)
{
  return argv[1][5] - '0';
}


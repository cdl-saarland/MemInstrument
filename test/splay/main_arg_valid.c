// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -memsafety-genchecks %t0.ll -memsafety-imechanism=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 foo0 2> %t3.err
// RUN: %not fgrep "non-existing witness" %t3.err

int main(int argc, char** argv)
{
  return argv[1][3] - '0';
}


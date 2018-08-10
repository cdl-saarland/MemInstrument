// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %t2 foo0 2> %t3.err

int main(int argc, char** argv)
{
  int i = 0;
  for (char* c = argv[1]; *c; ++c) {
    i = *c;
  }
  return i - '0';
}


// RUN: %clang -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
// RUN: %not --crash %t2 foo0 2> %t3.err
// RUN: fgrep "Memory safety violation!" %t3.err

int main(int argc, char** argv)
{
  return argv[2][3] - '0';
}


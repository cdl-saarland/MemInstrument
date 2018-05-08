// RUN: %clang -O0 -Xclang -disable-O0-optnone -x c++ -emit-llvm -c -S -o %t0.ll %s
// RUN: %opt %loadlibs -mem2reg -instnamer -meminstrument %t0.ll -mi-config=splay -S > %t1.ll
// RUN: %clink -lstdc++ -ldl -l:libsplay.a -g -o %t2 %t1.ll
// RUN: %t2

class Foo {
public:
  virtual int doTheThing(int x) {
    return x;
  }
  int y;
};


class Bar : public Foo {
public:
  virtual int doTheThing(int x) override {
    return 2 * x;
  }
  int z;
};

Foo a;
Bar b;

int main(void) {
  Foo *ap = &a;
  Foo *bp = &b;

  int res1 = ap->doTheThing(42);
  int res2 = bp->doTheThing(21);

  return res1 - res2;
}

// RUN: %clang -g -O0 -fPIC -D LIB_VERSION=1  -shared -o %t0_lib.so %s
// RUN: %clang -g -O0 -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t1_main.ll %s
// RUN: %opt %loadlibs -mem2reg -meminstrument -mi-config=splay %t1_main.ll -S > %t2_main.ll
// RUN: %clink -g -O0 -L/ -l:%t0_lib.so -ldl -l:libsplay.a -o %t3 %t2_main.ll
// RUN: %t3
// This fails because we cannot add the external static data to the splay tree.

struct T {
  int y;
  int z;
};

struct S {
  int x;
  struct T* t;
};

extern struct S* s;

extern void init(void);

#ifdef LIB_VERSION

struct S gs;
struct T gt;
struct S* s;

void init(void) {
  gs.x = 42;
  gs.t = &gt;
  gt.y = 20;
  gt.z = 22;
  s = &gs;
}

#else

int main(void) {
  init();
  return s->x - (s->t->y + s->t->z);
}

#endif

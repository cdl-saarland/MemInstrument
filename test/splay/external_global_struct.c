// RUN: %clang -g -O0 -fPIC -D LIB_VERSION=1  -shared -o %t0_lib.so %s
// RUN: %clang -g -O0 -Xclang -disable-O0-optnone -emit-llvm -c -S -o %t1_main.ll %s
// RUN: %opt %loadlibs %preppasses -meminstrument -mi-config=splay %t1_main.ll -S > %t2_main.ll
// RUN: %clink -g -O0 -L/ -l:%t0_lib.so -ldl -l:libsplay.a -o %t3 %t2_main.ll
// RUN: %t3

#ifdef LIB_VERSION

struct S {
  int x;
  int y;
};

struct S s;

void foo(struct S* sl) {
  sl->x = 42;
  sl->y = 17;
}

#else

struct S;

extern struct S s;

void foo(struct S* sl);

int main(void) {
  foo(&s);
  return 0;
}

#endif


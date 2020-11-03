// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1  %s -mllvm -mi-config=softbound -mllvm -mi-mode=gatheritargets -mllvm -debug-only=meminstrument-itargetprovider -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: dereference check with constant size 8B for y at entry::[{{.*}},2]
// CHECK-NEXT: dereference check with constant size 4B{{.*}}at entry::[{{.*}},3]
// CHECK-NEXT: dereference check with constant size 4B for x at entry::[store,5]
// CHECK-NEXT: dereference check with constant size 8B for y at entry::[{{.*}},6]
// CHECK-NEXT: value invariant{{.*}}at entry::[ret,7]

// CHECK: call invariant{{.*}}entry::call
// CHECK-NEXT: dereference check with constant size 4B for call at entry

// REQUIRES: asserts

int x;
int *y = &x;

int *f() {
  x = 17 + *y;
  return y;
}

int main() {
  return *f();
}

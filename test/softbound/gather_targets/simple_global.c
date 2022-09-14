// RUN: %clang -c -S -fplugin=%passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=gatheritargets -mllvm -debug-only=meminstrument-itargetprovider -emit-llvm -o - 2>&1 | %filecheck %s

// CHECK: dereference check with constant size 4B for x at entry

// REQUIRES: asserts

int x;

int main() {
  return x;
}

// RUN: %clang -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=genwitnesses -mllvm -debug-only=softbound -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: Insert witness {{.*}}intermediate target for x at entry
// CHECK-NEXT: Instrumentee: @x
// CHECK-NEXT: Created bounds:
// CHECK-NEXT: Base: {{.*}} @x to i8*
// CHECK-NEXT: Bound: {{.*}} getelementptr inbounds (i32, i32* @x, i32 1)

// REQUIRES: asserts

int x;

int main() {
  return x;
}

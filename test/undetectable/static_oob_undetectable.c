// RUN: %clang -S -c -emit-llvm -O1 %s -o - 2>&1 | %filecheck %s

// CHECK-NOT: load

static int Ar[3];

int main(int argc, char const *argv[]) {
    return Ar[3];
}

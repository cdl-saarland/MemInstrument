// RUN: %clang -g -Xclang -load -Xclang %passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t 1 1

const int some_thing[5] = {2, 4, 6, 8, 10};
int another_thing[5];

int main(int argc, char const *argv[]) {
    another_thing[1] = some_thing[argc];
    return another_thing[argc];
}

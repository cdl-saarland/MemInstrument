// RUN: %clang -fplugin=%passlib -mcmodel=large -O1 %s -mllvm -mi-config=lowfat -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t 1 1

static int some_thing[5];

int main(int argc, char const *argv[]) {
    some_thing[2] = 1;
    return some_thing[argc];
}

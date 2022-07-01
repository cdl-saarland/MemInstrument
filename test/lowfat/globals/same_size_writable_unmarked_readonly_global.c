// RUN: %clang -mcmodel=large -O1 %s -emit-llvm -S -o %t.ll
// RUN: %clang -mcmodel=large %t.ll %linklowfat -o %t
// RUN: %t 1 1

// XFAIL: *
// For some reason, our custom section ends up being read-only.
// https://stackoverflow.com/questions/72817423/how-does-the-linker-determine-which-custom-sections-are-read-only

static double some_thing[5] __attribute__ ((section ("lf_section_64"))) = {2.0, 4.0, 6.0, 8.0, 10.0};
static double another_thing[5]  __attribute__ ((section ("lf_section_64")));

int main(int argc, char const *argv[]) {
    another_thing[1] = some_thing[argc];
    return another_thing[argc] == 0.0;
}

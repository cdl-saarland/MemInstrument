// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdlib.h>
#include <sys/mman.h>

int main(void) {
  int *p = (int*) mmap(NULL, 10 * sizeof(int), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  for (int i = 0; i < 10; ++i) {
      p[i] = i;
  }
  int res = p[3];
  munmap(p, 10 * sizeof(int));

  return res - 3;
}


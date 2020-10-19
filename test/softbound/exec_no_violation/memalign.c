// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

#include <stdlib.h>
#include <malloc.h>

int main(void) {
  void *a = NULL;
  posix_memalign(&a, 16, 64);
  void *b = memalign(16, 64);
  void *c = aligned_alloc(16, 64);
  void *d = valloc(16);
  void *e = pvalloc(16);

  int *aW = (int *) a;
  *aW = 5;
  aW++;
  *aW = 6;

  int *bW = (int *) b;
  *bW = 6;

  int *cW = (int *) c;
  *cW = 6;

  int *dW = (int *) d;
  *dW = 6;

  int *eW = (int *) e;
  *eW = 6;

  int res = *aW + *bW + *cW + *dW+ *eW;

  free(a);
  free(b);
  free(c);
  free(d);
  free(e);

  printf("Result: %i\n", res);
  return 0;
}

// RUN: %clang -O1 -c -S -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound -mllvm -mi-mode=setup -mllvm -stats -emit-llvm -o - 2>&1 | %fileCheck %s

// CHECK: 39? softbound{{.*}}Number of metadata stores inserted
// TODO check that the number of stores is correct

// XFAIL: *

#include <stdio.h>
#include <stdlib.h>

int x = 5;
int y = 12;
int z = 17;
int *p = &x;
int *q = &y;

int ArSizeFive[5];

typedef struct {
  int *ptr;
  int **ptrptr;
  int i;
} PtrStruct;

typedef struct {
  int *Ar[2];
  int i;
} ArOfPtrStruct;

typedef struct {
  PtrStruct PS[2];
  ArOfPtrStruct ArPS[2];
} MoreStructNesting;

typedef struct {
  int *ptr;
  PtrStruct PS[2];
  ArOfPtrStruct ArPS;
  MoreStructNesting StructNest[2];
  PtrStruct *PtrPS;
} NestedStruct;


PtrStruct threeElem = {&x, &p, 2};

ArOfPtrStruct ptrArray = {{&ArSizeFive[1], &z}, 3};

MoreStructNesting moreNested = {{{&y, &p, 1}, {&z, &q, 2}}, {{{&ArSizeFive[2], &ArSizeFive[3]}, 3}, {{&x, &y}, 4}}};

NestedStruct full = {
  &x,
  {{&x, &p, -2}, {&y, &q, -1}},
  {{&ArSizeFive[4], &z}, 21},
  {{{{&y, &p, 1}, {&z, &q, 2}}, {{{&ArSizeFive[0], &ArSizeFive[1]}, -3}, {{&x, &y}, -4}}}, {{{&y, &p, -5}, {&z, &q, -6}}, {{{&ArSizeFive[2], &ArSizeFive[3]}, -7}, {{&x, &y}, -8}}}},
  &threeElem
};

int main(int argc, char **argv) {

  // Print the addresses of the simple int/int * global variables
  printf("Basics:\n\t&x %p \n\t&y %p \n\t&z %p \n\t&p %p \n\t&q %p \n\t p %p \n\t q %p \n\tArSizeFive %p\n\n", &x, &y, &z, &p, &q, p, q, ArSizeFive);

  // Access the most basic struct
  printf("threeElem:\n\tptr: %p \n\t*ptr: %i \n\tptrptr: %p \n\t*ptrptr: %p \n\t**ptrptr: %i \n\ti: %i\n\n", threeElem.ptr, *threeElem.ptr, threeElem.ptrptr, *threeElem.ptrptr, **threeElem.ptrptr, threeElem.i);

  // The original SoftBound implementation will crash here with missing bounds (this print is otherwise not necessary and only a subset of the next one)
  printf("ptrArray:\n\t*Ar[0] %i\n\n", *ptrArray.Ar[0]);

  // Access the struct that contains an array with pointers
  printf("ptrArray:\n\tAr[0] %p \n\t*Ar[0] %i\n\tAr[1] %p \n\t*Ar[1] %i \n\t i: %i\n\n", ptrArray.Ar[0], *ptrArray.Ar[0], ptrArray.Ar[1], *ptrArray.Ar[1], ptrArray.i);

  // Access the struct that contains the first two (twice each); request only some elements
  printf("moreNested:\n\tPS[1].ptrptr %p \n\t*PS[1].ptrptr %p \n\t**PS[1].ptrptr %i \n\tArPS[0].i %i \n\tArPS[0].Ar[1] %p \n\t*ArPS[0].Ar[1] %i\n\n", moreNested.PS[1].ptrptr, *moreNested.PS[1].ptrptr, **moreNested.PS[1].ptrptr, moreNested.ArPS[0].i, moreNested.ArPS[0].Ar[1], *moreNested.ArPS[0].Ar[1]);

  // Access the "full" struct (some elements only)
  printf("full:\n\tPS[0].ptr %p \n\tPS[1].ptrptr %p \n\t**PS[1].ptrptr %i \n\tArPS.i %i \n\t*ArPS.Ar[1] %i \n\tStructNest[1].ArPS[0].Ar[1] %p \n\t*StructNest[1].ArPS[1].Ar[1] %i\n\tPtrPS %p \n\tPtrPS->i %i\n\t**(PtrPS->ptrptr) %i \n", full.PS[0].ptr, full.PS[1].ptrptr, **full.PS[1].ptrptr, full.ArPS.i, *full.ArPS.Ar[1], full.StructNest[1].ArPS[0].Ar[1], *full.StructNest[1].ArPS[1].Ar[1], full.PtrPS, full.PtrPS->i, **(full.PtrPS->ptrptr));
  return 0;
}

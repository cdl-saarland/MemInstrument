// RUN: %clink -fplugin=%passlib -mllvm -mi-config=rt-stat -mllvm -mi-opt-pico -mllvm -placeMeta -ldl -l:librt_stat.a -g -O1 -o %t0 %s
// RUN: %t0 2> %t1.log
// RUN: grep -e "\.c - l 12 - c 10 - marked store .* : 20" %t1.log
// RUN: grep -e "\.c - l 18 - c 12 - marked load .* : 20" %t1.log
// REQUIRES: pico
// XFAIL: *

int main(void) {

  int A [20];

  for (int i = 0; i < 20; ++i) {
    A[i] = 2 * i - 1;
  }

  int res = 0;

  for (int i = 19; i >= 0; --i) {
    res += A[i];
  }

  return res - 360;
}

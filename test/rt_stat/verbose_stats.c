// RUN: export MI_CONFIG=rt_stat; %clink -Xclang -load -Xclang %passlib -ldl -l:librt_stat.a -g -O2 -o %t0 %s
// RUN: %t0 2> %t1.log
// RUN: grep -e "\.c:12: wild store: .* : 20" %t1.log
// RUN: grep -e "\.c:18: wild load: .* : 20" %t1.log
// REQUIRES: no_pmda

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

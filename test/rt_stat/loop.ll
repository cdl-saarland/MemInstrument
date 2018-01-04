; RUN: %opt %loadlibs -meminstrument -mi-config=rt_stat -mi-verbose=0 -S %s > %t1.ll
; RUN: %clink -ldl -l:librt_stat.a -o %t2 %t1.ll
; RUN: %t2 2> %t3.stats
; RUN: egrep "unmarked loads.*: 43" %t3.stats
; RUN: egrep "unmarked stores.*: 1" %t3.stats
; RUN: egrep "marked stores.*: 42" %t3.stats

define i32 @main() {
test_bb:
  %p = alloca i32, i64 12

  store i32 42, i32* %p
  br label %loop_header

loop_header:
  %x = load i32, i32* %p
  %cond = icmp sle i32 %x, 0
  br i1 %cond, label %loop_end, label %loop_body

loop_body:
  %xi = sub i32 %x, 1
  store i32 %xi, i32* %p, !nosanitize !1
  br label %loop_header

loop_end:
  ret i32 %x
}

!1 = !{i32 1}

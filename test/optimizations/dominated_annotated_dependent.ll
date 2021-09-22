; RUN: %opt -S  -load %passlib -meminstrument -mi-config=splay -mi-opt-dominance -mi-opt-annotation -stats %s 2>&1 | %filecheck %s

; CHECK: 1 {{.*}} checks filtered by annotation
; CHECK: 2 {{.*}} discarded because of dominating subsumption
; CHECK-NOT: 1 {{.*}} dereference checks inserted

define i32 @f(i32* %p, i32 %x) {
entry:
  br label %test_bb

test_bb:
  %cond = icmp sle i32 1, 0
  br i1 %cond, label %loop_end, label %loop_body

loop_body:
  %xi = sub i32 %x, 1
  %xj = sub i32 %x, 2
  %xk = sub i32 %x, 3
  store i32 %xi, i32* %p, !nosanitize !1
  store i32 %xj, i32* %p
  store i32 %xk, i32* %p
  br label %test_bb

loop_end:
  ret i32 %x
}

!1 = !{i32 1}

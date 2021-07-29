; RUN: %opt -S -load %passlib -meminstrument -mi-config=splay -mi-opt-dominance -mi-opt-annotation -stats %s 2>&1 | %fileCheck %s

; CHECK: 1 {{.*}} checks filtered by annotation
; CHECK: 1 {{.*}} discarded because of dominating subsumption
; CHECK: 1 {{.*}} dereference checks inserted

define i32 @f(i32* %p, i32* %q) {
entry:
  %x = load i32, i32* %q, align 4
  br label %test_bb

test_bb:
  %x1 = load i32, i32* %q, align 4
  %cond = icmp sle i32 %x1, 0
  br i1 %cond, label %loop_end, label %loop_body

loop_body:
  %xi = sub i32 %x, 1
  store i32 %xi, i32* %p, !nosanitize !1
  br label %test_bb

loop_end:
  ret i32 %x
}

!1 = !{i32 1}

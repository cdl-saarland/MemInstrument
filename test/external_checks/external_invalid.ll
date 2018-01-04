; RUN: %opt %loadlibs -meminstrument %s -mi-config=splay -mi-use-extchecks -debug-only=meminstrument -S > %t1.ll 2> %t2.log
; RUN: fgrep "call void @__splay_fail_simple" %t1.ll
; RUN: fgrep "updating ITargets with pass" %t2.log
; RUN: fgrep "generating external checks with pass" %t2.log
; RUN: %clink -ldl -l:libsplay.a -o %t3 %t1.ll
; RUN: %not %t3 2> %t4.log
; RUN: fgrep "Memory safety violation" %t4.log

define i32 @main() {
test_bb:
  %p = alloca i32, i64 12

  %pp = getelementptr i32, i32* %p, i64 16

  %p2 = call i32* @foo(i32* %pp)

  %r = load i32, i32* %p2
  ret i32 %r
}

define i32* @foo(i32* %p) {
foo_start:
  br label %foo_bb

foo_bb:
  store i32 0, i32* %p, !checkearly !0
  ret i32* %p
}

!0 = !{!"foo"}

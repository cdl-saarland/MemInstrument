; RUN: %opt %loadlibs -meminstrument %s -mi-config=softbound -S

; XFAIL: *

define i32 @test(i32* %p) {
bb:
  %p1 = getelementptr i32, i32* %p, i64 0
  %x1 = load i32, i32* %p1
  ret i32 42
}


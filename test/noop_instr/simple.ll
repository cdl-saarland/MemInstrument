; RUN: %opt %loadlibs -meminstrument %s -mi-config=noop -mi-random-filter-ratio=0.4 -mi-random-filter-seed=424344 -S > %t1.ll
; RUN: egrep "store volatile .* @mi_check_result_location" %t1.ll | wc -l | grep 6
; RUN: clang -O3 -c -S -o %t2.s %t1.ll
; RUN: egrep "\<ja\>" %t2.s | wc -l | grep 12

declare void @foo(i32)

define i32 @test(i32* %p) {
bb:
  %p1 = getelementptr i32, i32* %p, i64 0
  %x1 = load i32, i32* %p1

  %p2 = getelementptr i32, i32* %p, i64 1
  %x2 = load i32, i32* %p2

  %p3 = getelementptr i32, i32* %p, i64 2
  %x3 = load i32, i32* %p3

  %p4 = getelementptr i32, i32* %p, i64 3
  %x4 = load i32, i32* %p4

  %p5 = getelementptr i32, i32* %p, i64 4
  %x5 = load i32, i32* %p5

  %p6 = getelementptr i32, i32* %p, i64 5
  %x6 = load i32, i32* %p6

  %p7 = getelementptr i32, i32* %p, i64 6
  %x7 = load i32, i32* %p7

  %p8 = getelementptr i32, i32* %p, i64 7
  %x8 = load i32, i32* %p8

  %p9 = getelementptr i32, i32* %p, i64 8
  %x9 = load i32, i32* %p9

  %p10 = getelementptr i32, i32* %p, i64 9
  %x10 = load i32, i32* %p10

  call void @foo(i32 %x1)
  call void @foo(i32 %x2)
  call void @foo(i32 %x3)
  call void @foo(i32 %x4)
  call void @foo(i32 %x5)
  call void @foo(i32 %x6)
  call void @foo(i32 %x7)
  call void @foo(i32 %x8)
  call void @foo(i32 %x9)
  call void @foo(i32 %x10)

  ret i32 42
}


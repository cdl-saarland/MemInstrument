; RUN: %opt %loadlibs -meminstrument -mi-config=splay -mi-use-filters -mi-mode=genwitnesses -debug-only=meminstrument-genwitnesses %s > /dev/null 2> %t.log
; RUN: %not fgrep "(ptr, bb::five)" < %t.log

define i64 @test(i64* %ptr) #0 {
bb:
  store i64 5, i64* %ptr
  %five = load i64, i64* %ptr
  ret i64 %five
}

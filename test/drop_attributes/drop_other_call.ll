; RUN: %opt %loadlibs -meminstrument %s -S 2>&1 | %filecheck %s

; CHECK: foo
; CHECK-NOT: argmemonly

declare i32 @bar()

define i32 @foo(i32* %p) #3 {
  %val = call i32 @bar()
  ret i32 %val
}

define i32 @main() {
entry:
  %tmp = alloca i32, align 4
  store i32 42, i32* %tmp
  %val = call i32 @foo(i32* %tmp)
  ret i32 %val
}

attributes #3 = { argmemonly }



; RUN: %opt -load %passlib -memsafety-genwitnesses -debug-only=meminstrument-gatheritargets %s > /dev/null 2> %t.log

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@bar = global i32 42, align 4

define void @foo() {
entry:
  store i32 42, i32* getelementptr inbounds (i32, i32* @bar, i64 3), align 4
  ret void
}


; RUN: %not %opt %loadlibs -mem2reg -meminstrument %s -mi-config=splay -S 2>&1 | %filecheck %s

; CHECK: {{.*}}Meminstrument Error{{.*}}lifetime intrinsic

declare void @llvm.lifetime.start(i64, i8* nocapture) #3
declare void @llvm.lifetime.end(i64, i8* nocapture) #3

define i32 @main() {
entry:
  %tmp = alloca [32 x i8], align 16
  %tmp3 = getelementptr inbounds [32 x i8], [32 x i8]* %tmp, i64 0, i64 0
  call void @llvm.lifetime.start(i64 32, i8* nonnull %tmp3) #10

  %tmp4 = getelementptr inbounds [32 x i8], [32 x i8]* %tmp, i64 0, i64 7

  store i8 42, i8* %tmp4, align 1

  call void @llvm.lifetime.end(i64 32, i8* nonnull %tmp3) #10

  ret i32 0
}

attributes #3 = { argmemonly nounwind }

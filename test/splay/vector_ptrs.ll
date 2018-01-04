; RUN: %opt %loadlibs -meminstrument %s -mi-config=splay -S > %t1.ll
; RUN: %clink -ldl -l:libsplay.a -o %t2 %t1.ll
; RUN: %t2
; XFAIL: *

define i32 @test(i64 %n, i32* %p) {
bb:
  %p1 = alloca i32
  %p2 = alloca i32, i64 8
  %p3 = alloca i32, i64 2
  %call = call noalias i8* @malloc(i64 16)
  %p4 = bitcast i8* %call to i32*
  store i32 0, i32* %p4

  %v1 = insertelement <4 x i32*> < i32* null, i32* null, i32* null, i32* null >, i32* %p1, i32 0
  %v2 = insertelement <4 x i32*> %v1, i32* %p2, i32 1
  %v3 = insertelement <4 x i32*> %v2, i32* %p3, i32 2
  %v4 = insertelement <4 x i32*> %v3, i32* %p4, i32 3

  %vs = shufflevector <4 x i32*> %v4, <4 x i32*> %v4, <3 x i32> <i32 7, i32 3, i32 1>

  %call2 = call noalias i8* @malloc(i64 12)
  %mem = bitcast i8* %call2 to <3 x i32*>*

  store <3 x i32*> %vs, <3 x i32*>* %mem

  %vres = load <3 x i32*>, <3 x i32*>* %mem

  %pres = extractelement <3 x i32*> %vres, i32 0

  %x = load i32, i32* %pres
  ret i32 %x
}

declare noalias i8* @malloc(i64)

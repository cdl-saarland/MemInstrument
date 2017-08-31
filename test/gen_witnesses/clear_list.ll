; RUN: %opt %loadlibs -memsafety-genwitnesses -debug-only=meminstrument-genwitnesses %s > /dev/null 2> %t.log

%struct.List = type { %struct.ListItem* }
%struct.ListItem = type { %struct.ListItem*, i8* }

; Function Attrs: noreturn nounwind
declare void @__assert_fail() local_unnamed_addr #4

; Function Attrs: nounwind
declare void @free(i8* nocapture) local_unnamed_addr #2

define internal void @clearList(%struct.List* %arg) #1 {
bb:
  %tmp = icmp eq %struct.List* %arg, null
  br i1 %tmp, label %bb7, label %bb1

bb1:                                              ; preds = %bb
  %tmp2 = getelementptr inbounds %struct.List, %struct.List* %arg, i64 0, i32 0
  %tmp3 = bitcast %struct.List* %arg to i64*
  %tmp4 = load %struct.ListItem*, %struct.ListItem** %tmp2, align 8
  %tmp5 = icmp eq %struct.ListItem* %tmp4, null
  br i1 %tmp5, label %bb16, label %bb6

bb6:                                              ; preds = %bb1
  br label %bb8

bb7:                                              ; preds = %bb
  tail call void @__assert_fail() #12
  unreachable

bb8:                                              ; preds = %bb8, %bb6
  %tmp9 = phi %struct.ListItem* [ %tmp13, %bb8 ], [ %tmp4, %bb6 ]
  %tmp10 = bitcast %struct.ListItem* %tmp9 to i64*
  %tmp11 = load i64, i64* %tmp10, align 8
  store i64 %tmp11, i64* %tmp3, align 8
  %tmp12 = bitcast %struct.ListItem* %tmp9 to i8*
  tail call void @free(i8* %tmp12) #10
  %tmp13 = load %struct.ListItem*, %struct.ListItem** %tmp2, align 8
  %tmp14 = icmp eq %struct.ListItem* %tmp13, null
  br i1 %tmp14, label %bb15, label %bb8

bb15:                                             ; preds = %bb8
  br label %bb16

bb16:                                             ; preds = %bb15, %bb1
  ret void
}

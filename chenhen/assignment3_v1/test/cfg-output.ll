; ModuleID = 'cfg-output.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32-S128"
target triple = "i386-pc-linux-gnu"

; Function Attrs: nounwind readnone
define i32 @compare(i32 %a, i32 %b) #0 {
entry:
  %cmp = icmp sgt i32 %a, %b
  %a.b = select i1 %cmp, i32 %a, i32 %b
  ret i32 %a.b
}

; Function Attrs: nounwind readnone
define i32 @main() #0 {
entry:
  ret i32 0
}

attributes #0 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.4 (tags/RELEASE_34/final)"}

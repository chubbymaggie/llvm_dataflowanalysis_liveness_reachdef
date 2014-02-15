; ModuleID = 'test1.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32-S128"
target triple = "i386-pc-linux-gnu"

; Function Attrs: nounwind
define i32 @sum(i32 %a, i32 %b, i32 %c) #0 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %c.addr = alloca i32, align 4
  %d = alloca i32, align 4
  %e = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  store i32 %c, i32* %c.addr, align 4
  %0 = load i32* %c.addr, align 4
  %cmp = icmp sgt i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32* %a.addr, align 4
  store i32 %1, i32* %d, align 4
  %2 = load i32* %b.addr, align 4
  store i32 %2, i32* %e, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %3 = load i32* %b.addr, align 4
  store i32 %3, i32* %d, align 4
  %4 = load i32* %a.addr, align 4
  store i32 %4, i32* %e, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %5 = load i32* %d, align 4
  %6 = load i32* %e, align 4
  %add = add nsw i32 %5, %6
  ret i32 %add
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.4 (tags/RELEASE_34/final)"}

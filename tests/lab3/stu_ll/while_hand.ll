; ModuleID = 'while.c'
source_filename = "while.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
;main函数
define dso_local i32 @main() #0 {
entry:

%a = alloca i32, align 4
%i = alloca i32, align 4
;int a  int i
store i32 10, i32* %a, align 4
store i32 0, i32* %i, align 4
;a = 10 i = 0
br label %while.cond

while.cond:                                       
  %0 = load i32, i32* %i, align 4
  %cmp = icmp slt i32 %0, 10
  ;compare i with 10
  br i1 %cmp, label %while.body, label %while.end


while.body:                                      
  %1 = load i32, i32* %i, align 4
  %inc = add  i32 %1, 1
  store i32 %inc, i32* %i, align 4
  ; i = i + 1
  %2 = load i32, i32* %a, align 4
  %3 = load i32, i32* %i, align 4
  %add = add  i32 %3, %2
  store i32 %add, i32* %a, align 4
  ; a = a = i
  br label %while.cond



while.end:                                        
  %4 = load i32, i32* %a, align 4
  ret i32 %4
  ;return a
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}

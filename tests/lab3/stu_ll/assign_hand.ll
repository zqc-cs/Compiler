; ModuleID = 'assign.c'
source_filename = "assign.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
;注释：main函数的定义
define dso_local i32 @main() #0 {
entry:
    %a = alloca [10 x i32] , align 16
    ;申请数组a[]的内存，大小为10*i32
    %pa0 = getelementptr [10 x i32] , [10 x i32]* %a, i64 0, i64 0
    ;指向a[0]的指针，详见报告问题3
    store i32 10, i32* %pa0, align 16
    ;a[0] = 10
    %pa1 = getelementptr [10 x i32] , [10 x i32]* %a, i64 0, i64 1
    ;指向a[1]的数组
    %a0 = load i32, i32* %pa0, align 16
    ;%a0 = a[0]
    %a02 = mul i32 %a0, 2
    store i32 %a02, i32* %pa1, align 16
    ;a[1] = a[0] * 2
    %a1 = load i32, i32* %pa1, align 16
    ret i32 %a1
    ;return a[1]
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}

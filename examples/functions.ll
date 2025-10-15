; ModuleID = 'void_module'
source_filename = "void_module"

@0 = private unnamed_addr constant [10 x i8] c"mult: %d\0A\00", align 1
@1 = private unnamed_addr constant [9 x i8] c"add: %d\0A\00", align 1
@2 = private unnamed_addr constant [9 x i8] c"sub: %d\0A\00", align 1

define i32 @add(i32 %x, i32 %y) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %y2 = alloca i32, align 4
  store i32 %y, ptr %y2, align 4
  %x3 = load i32, ptr %x1, align 4
  %y4 = load i32, ptr %y2, align 4
  %addtmp = add i32 %x3, %y4
  ret i32 %addtmp
}

define i32 @sub(i32 %x, i32 %y) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %y2 = alloca i32, align 4
  store i32 %y, ptr %y2, align 4
  %x3 = load i32, ptr %x1, align 4
  %y4 = load i32, ptr %y2, align 4
  %subtmp = sub i32 %x3, %y4
  ret i32 %subtmp
}

define void @main() {
entry:
  %operation = alloca ptr, align 8
  store ptr @anon_0, ptr %operation, align 8
  %operation1 = load ptr, ptr %operation, align 8
  %0 = call i32 %operation1(i32 10, i32 2)
  %1 = call i32 (ptr, ...) @printf(ptr @0, i32 %0)
  store ptr @add, ptr %operation, align 8
  %operation2 = load ptr, ptr %operation, align 8
  %2 = call i32 %operation2(i32 10, i32 2)
  %3 = call i32 (ptr, ...) @printf(ptr @1, i32 %2)
  store ptr @sub, ptr %operation, align 8
  %operation3 = load ptr, ptr %operation, align 8
  %4 = call i32 %operation3(i32 10, i32 2)
  %5 = call i32 (ptr, ...) @printf(ptr @2, i32 %4)
  ret void
}

define internal i32 @anon_0(i32 %x, i32 %y) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %y2 = alloca i32, align 4
  store i32 %y, ptr %y2, align 4
  %x3 = load i32, ptr %x1, align 4
  %y4 = load i32, ptr %y2, align 4
  %multmp = mul i32 %x3, %y4
  ret i32 %multmp
}

declare i32 @printf(ptr, ...)

# lab3 实验报告
学号PB18000359   姓名 柳文浩

## 问题1: cpp与.ll的对应
请描述你的cpp代码片段和.ll的每个BasicBlock的对应关系。描述中请附上两者代码。

对于assign.c:

```cpp
int main() {
	auto module = new Module("Cminus code");  // module name是什么无关紧要
	auto builder = new IRBuilder(nullptr, module);
	Type *Int32Type = Type::get_int32_type(module);

	auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);//mian函数
	auto bb = BasicBlock::create(module, "entry", mainFun);
    //基本块bb的开始
	builder->set_insert_point(bb);
	
	auto *arrayType = ArrayType::get(Int32Type, 10);//int[10]的array type
	auto retAlloca = builder->create_alloca(Int32Type);
	auto aAlloca = builder->create_alloca(arrayType);
	auto a0GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)});
	builder->create_store(CONST_INT(10), a0GEP);//a[0] = 10

	auto a1GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)});

	auto a0Load = builder->create_load(a0GEP);

	auto mul = builder->create_imul(a0Load, CONST_INT(2));

	builder->create_store(mul, a1GEP);//a[1] = a[0] * 2

	auto a1Load = builder->create_load(a1GEP);//load a[1]

	builder->create_ret(a1Load);//return a[1]
	//基本块bb的结束
	std::cout << module->print();
 	delete module;
return 0;}
```

只有一个基本块，对应assign_hand.ll中的main函数的基本块（以label entry开头，以ret语句结束）基本块的范围见注释

cpp中的bb对应.ll中的entry

```
define i32 @main() {
;基本块entry的开始
entry:
  %0 = alloca i32
  %1 = alloca [10 x i32]
  %2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 0
  store i32 10, i32* %2
  %3 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 1
  %4 = load i32, i32* %2
  %5 = mul i32 %4, 2
  store i32 %5, i32* %3
  %6 = load i32, i32* %3
  ret i32 %6
  ;基本块entry的结束
}
```

对于fun.c:

```cpp

int main() {
	auto module = new Module("Cminus code");  // module name是什么无关紧要
	auto builder = new IRBuilder(nullptr, module);
	Type *Int32Type = Type::get_int32_type(module);
	
	//callee 函数的开始
	std::vector<Type *> Ints(1, Int32Type);
	
	//通过返回值类型与参数类型列表得到函数类型
	auto calleeFunTy = FunctionType::get(Int32Type, Ints);
	// 由函数类型得到函数
	auto calleeFun = Function::create(calleeFunTy,"callee", module);
	
	// BB的名字在生成中无所谓,但是可以方便阅读
	auto bb = BasicBlock::create(module, "entry", calleeFun);
    //基本块bb的开始
	builder->set_insert_point(bb);
	
	auto retAlloca = builder->create_alloca(Int32Type);
	auto aAlloca = builder->create_alloca(Int32Type);
	
	std::vector<Value *> args;  // 获取callee函数的形参,通过Function中的iterator
	for (auto arg = calleeFun->arg_begin(); arg != calleeFun->arg_end(); arg++) {
    args.push_back(*arg);   // * 号运算符是从迭代器中取出迭代器当前指向的元素
	}
	
	builder->create_store(args[0], aAlloca); //store parameter a
	auto aLoad = builder->create_load(aAlloca);
	auto mul = builder->create_imul(CONST_INT(2), aLoad);//2*a
	builder->create_ret(mul);//return 2 * a;
    //基本块bb的结束
	//main函数的开始
	auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
	bb = BasicBlock::create(module, "entry", mainFun);
    //基本块bb的开始
	builder->set_insert_point(bb);
	retAlloca = builder->create_alloca(Int32Type);
	builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0
	auto call = builder->create_call(calleeFun, {CONST_INT(110)});//调用callee函数，传入的参数为常数110
	builder->create_ret(call);
    //基本块bb的结束
	
	std::cout << module->print();
	delete module;
	return 0;
	
	
}
```

```
define i32 @callee(i32 %0) {
;基本块entry的开始
entry:
  %1 = alloca i32
  %2 = alloca i32
  store i32 %0, i32* %2
  %3 = load i32, i32* %2
  %4 = mul i32 2, %3
  ret i32 %4
  ;基本块entry的结束
}
define i32 @main() {
;基本块entry的开始
entry:
  %0 = alloca i32
  store i32 0, i32* %0
  %1 = call i32 @callee(i32 110)
  ret i32 %1
  ;基本块entry的结束
}
```

共定义了两个函数，基本块范围见cpp代码中的注释，cpp中的每个函数对应.ll文件中的每个函数定义，而每个函数中各有一个基本块（以label entry开头，以ret语句结束）

对每个函数，cpp中的bb对应.ll中的entry

对于if.c：

```cpp
int main() {
	auto module = new Module("Cminus code");  // module name是什么无关紧要
	auto builder = new IRBuilder(nullptr, module);
	Type *Int32Type = Type::get_int32_type(module);
	Type *FloatType = Type::get_float_type(module);
	//main函数
	auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
	auto bb = BasicBlock::create(module, "entry", mainFun);
	// 基本块bb的开始
	builder->set_insert_point(bb);
	auto retAlloca = builder->create_alloca(Int32Type); 
	auto aAlloca = builder->create_alloca(FloatType);
	builder->create_store(CONST_FP(5.555), aAlloca);//float a = 5.555;
	auto aaLoad = builder->create_load(aAlloca);
	auto fcmp = builder->create_fcmp_gt(aaLoad, CONST_FP(1.000000e+00)); //IRBuilder.h,line 61
	auto trueBB = BasicBlock::create(module, "trueBB", mainFun);    // true分支
	auto falseBB = BasicBlock::create(module, "falseBB", mainFun);  // false分支
	auto retBB = BasicBlock::create(
		module, "", mainFun);  // return分支,提前create,以便true分支可以br

	auto br = builder->create_cond_br(fcmp, trueBB, falseBB);  // 条件BR
    //基本块bb的结束
    //基本块trueBB的开始
	builder->set_insert_point(trueBB);
	builder->create_store(CONST_INT(233), retAlloca);//返回值为233
	builder->create_br(retBB);
    //基本块trueBB的结束
	//falseBB基本块的结束
	builder->set_insert_point(falseBB);
	builder->create_store(CONST_INT(0), retAlloca);//返回值为0
	builder->create_br(retBB);
    //falseBB基本块的结束
	//retBB基本块的开始
	builder->set_insert_point(retBB);  // ret分支
	auto retLoad = builder->create_load(retAlloca);  //load retAlloca
	builder->create_ret(retLoad);
    //retBB基本块的结束
	std::cout << module->print();
  delete module;
return 0;}
```

```
define i32 @main() {
;基本块entry的开始
entry:
  %0 = alloca i32
  %1 = alloca float
  store float 0x40163851e0000000, float* %1
  %2 = load float, float* %1
  %3 = fcmp ugt float %2,0x3ff0000000000000
  br i1 %3, label %trueBB, label %falseBB
  ;基本块entry的结束
  ;基本块trueBB的开始
trueBB:
  store i32 233, i32* %0
  br label %4
  ;基本块trueBB的结束
  ;基本块falseBB的开始
falseBB:
  store i32 0, i32* %0
  br label %4
  ;基本块falseBB的结束
  ;基本块4的开始
4:
  %5 = load i32, i32* %0
  ret i32 %5
  ;基本块4的结束
}

```

基本块范围见注释

对应关系：bb<->entry	trueBB<->trueBB		falseBB<->falseBB		retBB<->4

对于while.c：

```cpp
int main(){
	auto module = new Module("Cminus code");  // module name是什么无关紧要
	auto builder = new IRBuilder(nullptr, module);
	Type *Int32Type = Type::get_int32_type(module);
	//main函数
	auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
	auto bb = BasicBlock::create(module, "entry", mainFun);
    //bb基本块的开始
	builder->set_insert_point(bb);
	auto aAlloca = builder->create_alloca(Int32Type);
	auto iAlloca = builder->create_alloca(Int32Type);
	builder->create_store(CONST_INT(10), aAlloca);//a = 10;
	builder->create_store(CONST_INT(0), iAlloca);//i = 0;
	auto whilecond = BasicBlock::create(module, "whilecond", mainFun);    // true分支
	auto whilebody = BasicBlock::create(module, "whilebody", mainFun);  // false分支
	auto whileend = BasicBlock::create(
		module, "whileend", mainFun);  // whileend分支,提前create,以便true分支可以br
	builder->create_br(whilecond);
	//bb基本块的结束
	//whilecond基本块的开始
	builder->set_insert_point(whilecond); 
	auto iLoad = builder->create_load(iAlloca); 
	auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10)); //IRBuilder.h,line 26
	auto br = builder->create_cond_br(icmp, whilebody, whileend);
	//whilecond基本块的结束
	//whilebody基本块的开始
	builder->set_insert_point(whilebody); 
	iLoad = builder->create_load(iAlloca);
	auto inc = builder->create_iadd(iLoad, CONST_INT(1));
	builder->create_store(inc, iAlloca);
	auto aLoad = builder->create_load(aAlloca); 
	iLoad = builder->create_load(iAlloca); 
	auto add = builder->create_iadd(iLoad, aLoad);
	builder->create_store(add, aAlloca);
	builder->create_br(whilecond);
	//whilebody基本块的结束
	
	//whileend基本块的开始
	builder->set_insert_point(whileend); 
	aLoad = builder->create_load(aAlloca); 
	builder->create_ret(aLoad);
    //whileend基本块的结束
	std::cout << module->print();
  delete module;
return 0;}
```

```
define i32 @main() {
;entry基本块的开始
entry:
  %0 = alloca i32
  %1 = alloca i32
  store i32 10, i32* %0
  store i32 0, i32* %1
  br label %whilecond
  ;entry基本块的结束
  ;whilecond基本块的开始
whilecond:
  %2 = load i32, i32* %1
  %3 = icmp slt i32 %2, 10
  br i1 %3, label %whilebody, label %whileend
  ;whilecond基本块的结束
  ;whilebody基本块的开始
whilebody:
  %4 = load i32, i32* %1
  %5 = add i32 %4, 1
  store i32 %5, i32* %1
  %6 = load i32, i32* %0
  %7 = load i32, i32* %1
  %8 = add i32 %7, %6
  store i32 %8, i32* %0
  br label %whilecond
  ;whilebody基本块的结束
  ;whileend基本块的开始
whileend:
  %9 = load i32, i32* %0
  ret i32 %9
  ;whileend基本块的结束
}
```

基本块范围见注释

对应关系：bb<->entry	whilecond<->whilecond		whilebody<->whilebody		whileend<->whileend

## 问题2: Visitor Pattern
请指出visitor.cpp中，`treeVisitor.visit(exprRoot)`执行时，以下几个Node的遍历序列:numberA、numberB、exprC、exprD、exprE、numberF、exprRoot。  
序列请按如下格式指明：  
exprRoot->numberF->exprE->numberA->exprD

回答：

exprRoot->numberF->exprE->exprD->numberB->numberA->exprC->numberA->numberB

## 问题3: getelementptr
请给出`IR.md`中提到的两种getelementptr用法的区别,并稍加解释:
  - `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0` 

    这是对应于[10 x i32]的数组类型，第一个[10 x i32]表示基本类型是[10 x i32]，第二个[10 x i32]代表%1的类型，后面的第一个i32 0表示前进0个基本类型[10 x i32]，此时基本类型变为i32（去掉一层），第二个i32 %0表示前进%0 * i32,取得指针

  - `%2 = getelementptr i32, i32* %1 i32 %0` 

    这是对应i32*指针类型的写法，表示前进%0 * i32，取得指针

    

## 实验难点
描述在实验中遇到的问题、分析和解决方案

1. 读懂llvm的基本语法以及和C代码的对应关系（gcd_array.ll尤其复杂，难度大于4个实验题目，读懂后做实验显得得心应手）
2. 理解什么时候需要写内存和读内存，什么时候不需要
3. 对照注释阅读gcd_array_generator.cpp，自学c++一些基本语法，读懂各种函数用法（本实验中基本可以与.ll语句简单一一对应，降低了难度）
4. 注释的书写

## 实验反馈
吐槽?建议?

在核心类介绍.md文件中Type里ArrayType的介绍里

ArrayType

- 含义：数组类型
- 成员：
  - contained_：数组成员的类型
  - num_elements_：数组维数

中num_elements_翻译成数组维数容易引起误解，我理解应该是数组成员的个数



希望能延长实验时间，编译实验相比算法计网等课程显得过于仓促










#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到
	
	
int main() {
	auto module = new Module("Cminus code");  // module name是什么无关紧要
	auto builder = new IRBuilder(nullptr, module);
	Type *Int32Type = Type::get_int32_type(module);
	
	//callee function
	std::vector<Type *> Ints(1, Int32Type);
	
	//通过返回值类型与参数类型列表得到函数类型
	auto calleeFunTy = FunctionType::get(Int32Type, Ints);
	// 由函数类型得到函数
	auto calleeFun = Function::create(calleeFunTy,"callee", module);
	
	// BB的名字在生成中无所谓,但是可以方便阅读
	auto bb = BasicBlock::create(module, "entry", calleeFun);
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
	
	auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
	bb = BasicBlock::create(module, "entry", mainFun);
	builder->set_insert_point(bb);
	retAlloca = builder->create_alloca(Int32Type);
	builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0
	

	
	
	auto call = builder->create_call(calleeFun, {CONST_INT(110)});//调用callee函数，传入的参数为常数110
	builder->create_ret(call);
	
	std::cout << module->print();
	delete module;
	return 0;
	
	
}





























	
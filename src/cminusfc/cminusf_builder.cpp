#include "cminusf_builder.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_ZERO(type) \
    ConstantZero::get(var_type, module.get())


// You can define global variables here
// to store state
bool access_return_node = false;
// 用于存储表达式回传的存在scope里的变量名。比如return-stmt -> return expression
// 在return-stmt结点，会先访问子节点，子节点给expression赋值。该过程是递归的最终在return-stmt结点调用scope.find(expression)
std::string expression= "";
std::string simple_expression = "";
std::string additive_expression = "";
std::string term = "";
std::string factor = "";
std::string var_name = "";

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) { 
    for(auto decl : node.declarations){
        //std::cout << "id "<< decl->id<<" type "<< decl->type <<std::endl;
        decl->accept(*this);
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTNum &node) { 
    // 在这里，只有factor结点会用visit访问该结点
    if(node.type == TYPE_INT){
        factor = "@factor-int";
        if(scope.find(factor)){
            auto alloca1 = (AllocaInst *)scope.find(factor);
            builder->create_store(ConstantInt::get(node.i_val, module.get()), alloca1);
        }else{
            auto int_alloca = builder->create_alloca(Type::get_int32_type(module.get()));
            builder->create_store(ConstantInt::get(node.i_val, module.get()), int_alloca);
            scope.push(factor, int_alloca);
        }
    }else{
        factor = "@factor-float";
        if(scope.find(factor)){
            auto alloca1 = (AllocaInst *)scope.find(factor);
            builder->create_store(ConstantFP::get(node.f_val, module.get()), alloca1);
        }else{
            auto float_alloca = builder->create_alloca(Type::get_float_type(module.get()));
            builder->create_store(ConstantFP::get(node.f_val, module.get()), float_alloca);
            scope.push(factor, float_alloca);
        }
    }
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    // 为了区分全局声明和局部声明，通过builder得到当前的bb。如果bb已经定义，通过bb得到函数，函数里的声明是局部声明，否则属于全局声明
    auto bb = builder->get_insert_block();
    auto Ty_Int = Type::get_int32_type(module.get());
    auto Ty_Float = Type::get_float_type(module.get());
    if(bb){
        auto fun = bb->get_parent();
        if (fun) {
            // 局部声明
            if (node.type == TYPE_INT) {
                if (node.num == nullptr) {
                    // 如果不是数组
                    auto val_alloca = builder->create_alloca(Ty_Int);
                    scope.push(node.id, val_alloca);
                }else if(node.num->i_val > 0){
                    // 如果是数组，定义数组类型
                    auto ArrayTy = ArrayType::get(Ty_Int, node.num->i_val);
                    // 为数组分配空间
                    auto array_alloca = builder->create_alloca(ArrayTy);
                    scope.push(node.id, array_alloca);
                }else{
                    // error
                }
            }else if (node.type == TYPE_FLOAT) {
                if(node.num == nullptr) {
                    // 不是数组
                    auto val_alloc = builder->create_alloca(Ty_Float);
                    scope.push(node.id, val_alloc);
                }else if(node.num->i_val > 0){
                    // 数组
                    auto ArrayTy = ArrayType::get(Ty_Float, node.num->i_val);
                    auto array_alloca = builder->create_alloca(ArrayTy);
                    scope.push(node.id, array_alloca);
                }else{
                    // error
                }
            }
        }
    }else {
        // 全局声明
        if(node.type == TYPE_INT){
        auto initializer = ConstantZero::get(Ty_Int, module.get());
            if (node.num == nullptr){
                // int
                auto x = GlobalVariable::create(node.id, module.get(), Ty_Int, false, initializer);
                scope.push("@global-int" + node.id, x);
            }else {
                // 数组元素个数大于0
                if(node.num->i_val > 0){
                auto ArrayTy = ArrayType::get(Ty_Int, node.num->i_val);
                auto x = GlobalVariable::create(node.id, module.get(), ArrayTy, false, initializer);
                scope.push("@global-int-array" + node.id, x);
                }else{
                    // error
                }
            }
        }else if (node.type == TYPE_FLOAT){
            auto initializer = ConstantZero::get(Ty_Float, module.get());
            if(node.num == nullptr){
                // float
                auto x = GlobalVariable::create(node.id, module.get(), Ty_Float, false, initializer);
                scope.push("@global-float" + node.id, x);
            }else if (node.num->i_val > 0){
                // 数组
                auto ArrayFloat = ArrayType::get(Ty_Float, node.num->i_val);
                auto x = GlobalVariable::create(node.id, module.get(), ArrayFloat, false, initializer);
                scope.push("@global-float-array" + node.id, x);
            }
        }
    }
    // std::cout << "id "<<node.id<<" type "<< node.type <<" ty "<<node.num<<" num " <<std::endl;
    
    
}

void CminusfBuilder::visit(ASTFunDeclaration &node) { 
    // 根据id获取函数名，根据tyoe获取所属的类型0--int 1--float 2--void
    // std::cout<<"test -- "<<node.id<<" test -- "<<node.type<<" test -- "<<std::endl;
    auto fun_name = node.id;
    Type *Ty;
    auto TyVoid = Type::get_void_type(module.get());
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    auto TyInt32Ptr = Type::get_int32_ptr_type(module.get());
    auto TyFloatPtr = Type::get_float_ptr_type(module.get());
    FunctionType *fun_type;
    // 获取函数的返回类型
    
    // params为函数参数类型的容器，遍历参数结点，将所有参数的类型依次push进容器
    std::vector<Type *>params;
    for (auto param: node.params) {
        if(param->type == TYPE_INT){
            if(param->isarray == false){
                params.push_back(TyInt32);
            } else{
                params.push_back(TyInt32Ptr);
            }
        } else if (param->type == TYPE_FLOAT){
            if(param->isarray == false){
                params.push_back(TyFloat);
            } else{
                params.push_back(TyFloatPtr);
            }
            
        } else{
            params.push_back(TyVoid);
        }
    }
    if(node.type == TYPE_INT){
        fun_type = FunctionType::get(TyInt32, params);
    } else if (node.type == TYPE_FLOAT){
        fun_type = FunctionType::get(TyFloat, params);
    } else{
        fun_type = FunctionType::get(TyVoid, params);
    }
    // 构造函数类型、创建函数、创建基本块、设置插入点
    auto fun = Function::create(fun_type, fun_name, module.get());
    auto bb = BasicBlock::create(module.get(), fun_name, fun);
    builder->set_insert_point(bb);
    // 将函数加入全局作用域
    scope.push(fun_name, fun);
    // 创建该函数的作用域
    scope.enter();
    // 获取函数参数
    std::vector<Value *> args;
    for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++){
        args.push_back(*arg);
    }
    int i = 0;
    for (auto param: node.params) {
        // 对每个参数，为它们创建一个地址，再将参数值存储到该地址，用于后续的使用
        // 如果该参数是数组，分配指向指针的空间，否则根据类型分配空间
        if(param->isarray){
            // 如果参数类型是int
            if(param->type == TYPE_INT){
                // 分配int 指针
                auto arg_alloca = builder->create_alloca(TyInt32Ptr);
                builder->create_store(args[i], arg_alloca);
                scope.push(param->id, arg_alloca);
            }else if(param->type == TYPE_FLOAT) {
                // 参数类型是Float
                auto arg_alloca = builder->create_alloca(TyFloatPtr);
                builder->create_store(args[i], arg_alloca);
                scope.push(param->id, arg_alloca);
            }
        }else {
            // int 或者 float
            if(param->type == TYPE_INT){
                auto arg_alloca = builder->create_alloca(TyInt32);
                builder->create_store(args[i], arg_alloca);
                scope.push(param->id, arg_alloca);
            }else if (param->type == TYPE_FLOAT){
                auto arg_alloca = builder->create_alloca(TyFloat);
                builder->create_store(args[i], arg_alloca);
                scope.push(param->id, arg_alloca);
            }
        }
        // scope.push(param->id, args[i]);
        i++;
    }
    node.compound_stmt->accept(*this);
    // 如果没有return语句
    if(fun_type->get_return_type()->is_void_type()){
        // 解决多个ret void的问题
        // 通过判断当前块的最后一条指令是不是终止指令，即br或者ret，如果不是，就生成一个ret
        auto bb_cur_inster = builder->get_insert_block();
        if(bb_cur_inster->get_terminator()){

        }else{
            builder->create_void_ret();
        }
    }
    if(builder->get_insert_block()->empty()){
        builder->get_insert_block()->erase_from_parent();
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) {

 }

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    scope.enter();
    for (auto local_dec : node.local_declarations){
        local_dec->accept(*this);
    }
    for (auto statement : node.statement_list){
        statement->accept(*this);
    }
    scope.exit();
 }

void CminusfBuilder::visit(ASTExpressionStmt &node) {
     if(node.expression != nullptr){
         node.expression->accept(*this);
     }
}

void CminusfBuilder::visit(ASTSelectionStmt &node) { 
    node.expression->accept(*this);
    // 从expression获得判断条件
    auto cmp_alloca = (AllocaInst *)scope.find(expression);
    auto cmp_load = builder->create_load(cmp_alloca);
    
    auto trueBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    BasicBlock *falseBB;
    auto nextBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    // 考虑cmp_load的类型
    if(node.else_statement){
        falseBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
        if(cmp_load->get_load_type()->is_integer_type()){
            auto cmp = builder->create_icmp_ne(cmp_load, ConstantInt::get(0, module.get()));
            builder->create_cond_br(cmp, trueBB, falseBB);
        }else if(cmp_load->get_load_type()->is_float_type()){
            auto cmp = builder->create_fcmp_ne(cmp_load, ConstantFP::get(0, module.get()));
            builder->create_cond_br(cmp, trueBB, falseBB);
        }else{
            //error
        }
    }else{
        if(cmp_load->get_load_type()->is_integer_type()){
            auto cmp = builder->create_icmp_ne(cmp_load, ConstantInt::get(0, module.get()));
            builder->create_cond_br(cmp, trueBB, nextBB);
        }else if(cmp_load->get_load_type()->is_float_type()){
            auto cmp = builder->create_fcmp_ne(cmp_load, ConstantFP::get(0, module.get()));
            builder->create_cond_br(cmp, trueBB, nextBB);
        }else{
            //error
        }
    }
    builder->set_insert_point(trueBB);
    node.if_statement->accept(*this);
    
    if(!builder->get_insert_block()->get_terminator()){
        builder->create_br(nextBB);
    }
    if(node.else_statement){
        builder->set_insert_point(falseBB);
        node.else_statement->accept(*this);
        if(!builder->get_insert_block()->get_terminator()){
            builder->create_br(nextBB);
        }
    }
    builder->set_insert_point(nextBB);
}


void CminusfBuilder::visit(ASTIterationStmt &node) {
    auto beginBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    auto statementBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    auto endBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    // 进入while语句后，要进入expression所在的基本块
    builder->create_br(beginBB);
    builder->set_insert_point(beginBB);
    node.expression->accept(*this);
    // 从expression获得判断条件
    auto cmp_alloca = (AllocaInst *)scope.find(expression);
    auto cmp_load = builder->create_load(cmp_alloca);
    // 考虑cmp_load的类型
    if(cmp_load->get_load_type()->is_integer_type()){
        auto cmp = builder->create_icmp_ne(cmp_load, ConstantInt::get(0, module.get()));
        builder->create_cond_br(cmp, statementBB, endBB);
    }else if(cmp_load->get_load_type()->is_float_type()){
        auto cmp = builder->create_fcmp_ne(cmp_load, ConstantFP::get(0, module.get()));
        builder->create_cond_br(cmp, statementBB, endBB);
    }else{
        //error
    }
    builder->set_insert_point(statementBB);
    node.statement->accept(*this);
    if(!builder->get_insert_block()->get_terminator()){
        builder->create_br(beginBB);
    }
    
    builder->set_insert_point(endBB);
 }

void CminusfBuilder::visit(ASTReturnStmt &node) {
    //std::cout<<"return"<<std::endl;
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    // expression = "abc";
    // auto alloca1 = builder->create_alloca(TyInt32);
    // builder->create_store(ConstantInt::get(1, module.get()), alloca1);
    // scope.push(expression, alloca1);
    // 获取函数、函数返回值类型
    auto fun = builder->get_insert_block()->get_parent();
    auto ret_type = fun->get_return_type();
    // auto ret_BB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    // builder->create_br(ret_BB);
    // builder->set_insert_point(ret_BB);
    // 可能需要全局抛出异常!!!!!!!
    if(node.expression != nullptr && !ret_type->is_void_type()){
        node.expression->accept(*this);
        auto retval_alloca = (AllocaInst*)(scope.find(expression));
        // 如果该地址在scope里
        if(retval_alloca){
            auto Ty = retval_alloca->get_alloca_type();
            auto ret_load = builder->create_load(retval_alloca);
            if(ret_type->is_integer_type()){
                // 如果返回值类型是int
                if(Ty->is_integer_type()){
                    // 不需要进行类型转换
                    builder->create_ret(ret_load);
                }else if(Ty->is_float_type()){
                    // 需要进行类型转换
                    auto ret_val = ret_load;
                    auto val = builder->create_fptosi(ret_val, TyInt32);
                    builder->create_ret(val);
                }
            }else if(ret_type->is_float_type()){
                // 返回值类型是float
                if(Ty->is_integer_type()){
                    // 需要进行类型转换
                    auto val = builder->create_sitofp(ret_load, TyFloat);
                    builder->create_ret(val);
                }else if(Ty->is_float_type()){
                    // 不需要进行类型转换
                    auto ret_val = ret_load;
                    builder->create_ret(ret_val);
                }
            }
        }
    }else {
        builder->create_void_ret();
    }
    
 }

void CminusfBuilder::visit(ASTVar &node) {
    // 由于一开始没考虑接口，在AssignExpression中没有visit var结点，代码冗余
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    auto TyFloatPtr = Type::get_float_ptr_type(module.get());
    auto TyInt32Ptr = Type::get_int32_ptr_type(module.get());
    if(node.expression){
        // 数组还是指针？
        // 获取数组的地址
        
        // 访问express结点得到偏移量
        node.expression->accept(*this);
        // 从expression得到值
        auto offset_Alloca = (AllocaInst *)scope.find(expression);
        auto offset_Alloca_Type = offset_Alloca->get_alloca_type();
        auto offset_load = builder->create_load(offset_Alloca);
        FpToSiInst *offset;
        if(offset_Alloca_Type->is_float_type()){
            // 需要类型转换
            offset = builder->create_fptosi(offset_load, TyInt32);
            
        }else{
            offset = (FpToSiInst *)offset_load;
        }
        // 检查下标是不是大于等于0
        auto bbtrue = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
        auto bbfalse = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
        auto icmp = builder->create_icmp_ge(offset, ConstantInt::get(0, module.get()));
        builder->create_cond_br(icmp, bbtrue, bbfalse);
        builder->set_insert_point(bbfalse);
        auto neg_idx_except_fun = scope.find("neg_idx_except");
        builder->create_call(neg_idx_except_fun, {});
        if(builder->get_insert_block()->get_parent()->get_return_type()->is_integer_type()){
            builder->create_ret(ConstantInt::get(0, module.get()));
        }
        else if(builder->get_insert_block()->get_parent()->get_return_type()->is_float_type()){
            builder->create_ret(ConstantFP::get(0,module.get()));
        }
        else builder->create_void_ret();
        builder->set_insert_point(bbtrue);

        GetElementPtrInst *gep;
        auto alloca1 = (AllocaInst*)scope.find(node.id);
        LoadInst *alloca_load;
        Type *alloca_Type;
        if(alloca1){
            alloca_load = builder->create_load(alloca1);
            alloca_Type = alloca1->get_alloca_type();
            if(alloca_Type->is_array_type()){
                gep = builder->create_gep(alloca1, {ConstantInt::get(0, module.get()),offset});
            }else{
                gep = builder->create_gep(alloca_load, {offset});
            }
        }else if((GlobalVariable *)scope.find("@global-int-array" + node.id)){
            auto array = (GlobalVariable *)scope.find("@global-int-array" + node.id);
            gep = builder->create_gep(array, {ConstantInt::get(0, module.get()), offset});
        }else if((GlobalVariable *)scope.find("@global-float-array" + node.id)){
            auto array = (GlobalVariable *)scope.find("@global-float-array" + node.id);
            gep = builder->create_gep(array, {ConstantInt::get(0, module.get()), offset});
        }
        // 通过scope查询
        if(gep->get_element_type()->is_integer_type()){
            factor = "@factor-int";
            if(scope.find(factor)){
                auto factor_alloca = (AllocaInst *)scope.find(factor);
                auto gep_load = builder->create_load(gep);
                builder->create_store(gep_load, factor_alloca);
            }else{
                auto factor_alloca = builder->create_alloca(TyInt32);
                auto gep_load = builder->create_load(gep);
                builder->create_store(gep_load, factor_alloca);
                scope.push(factor, factor_alloca);
            }
        
        }else if(gep->get_element_type()->is_float_type()){
            factor = "@factor-float";
            if(scope.find(factor)){
                auto factor_alloca = (AllocaInst *)scope.find(factor);
                auto gep_load = builder->create_load(gep);
                builder->create_store(gep_load, factor_alloca);
            }else{
                auto factor_alloca = builder->create_alloca(TyFloat);
                auto gep_load = builder->create_load(gep);
                builder->create_store(gep_load, factor_alloca);
                scope.push(factor, factor_alloca);
            }
        }
    }
    else{
        // 如果是简单变量
        // auto alloca1 = (AllocaInst*)scope.find(node.id);
        // auto alloca_Type = alloca1->get_alloca_type();
        // factor为该变量名，如果存储进scope的是变量名加上一些特定字符，则需要修改
        // 数组还是指针？
        // 获取数组的地址
        auto alloca1 = (AllocaInst*)scope.find(node.id);
        GetElementPtrInst *gep;
        factor = node.id;
        if(alloca1){
            auto alloca_load = builder->create_load(alloca1);
            auto alloca_Type = alloca1->get_alloca_type();
            if(alloca_Type->is_array_type()){
                gep = builder->create_gep(alloca1, {ConstantInt::get(0, module.get()),ConstantInt::get(0, module.get())});
                if(gep->get_element_type()->is_integer_type()){
                    factor = "@factor-array-int";
                    if(scope.find(factor)){
                        auto factor_alloca = (AllocaInst *)scope.find(factor);
                        builder->create_store(gep, factor_alloca);
                    }else{
                        // 这里可能有问题，是不是只用考虑指针类型呢
                        //std::cout<<" int-create "<<std::endl;
                        auto factor_alloca = builder->create_alloca(TyInt32Ptr);
                        builder->create_store(gep, factor_alloca);
                        scope.push(factor, factor_alloca);
                    }
                }
                else if(gep->get_element_type()->is_float_type()){
                    
                    factor = "@factor-array-float";
                    if(scope.find(factor)){
                        auto factor_alloca = (AllocaInst *)scope.find(factor);
                        builder->create_store(gep, factor_alloca);
                    }else{
                        // 这里可能有问题，是不是只用考虑指针类型呢
                        auto factor_alloca = builder->create_alloca(TyFloatPtr);
                        builder->create_store(gep, factor_alloca);
                        scope.push(factor, factor_alloca);
                    }
                }
            }
            else if(alloca_Type->is_pointer_type()){
                gep = builder->create_gep(alloca_load, {ConstantInt::get(0, module.get())});
                if(gep->get_element_type()->is_integer_type()){
                    factor = "@factor-ptr-int";
                    if(scope.find(factor)){
                        auto factor_alloca = (AllocaInst *)scope.find(factor);
                        builder->create_store(gep, factor_alloca);
                    }else{
                        // 这里可能有问题，是不是只用考虑指针类型呢
                        auto factor_alloca = builder->create_alloca(TyInt32Ptr);
                        builder->create_store(gep, factor_alloca);
                        scope.push(factor, factor_alloca);
                    }
                }
                else if(gep->get_element_type()->is_float_type()){
                    factor = "@factor-ptr-float";
                    if(scope.find(factor)){
                        auto factor_alloca = (AllocaInst *)scope.find(factor);
                        builder->create_store(gep, factor_alloca);
                    }else{
                        // 这里可能有问题，是不是只用考虑指针类型呢
                        auto factor_alloca = builder->create_alloca(TyFloatPtr);
                        builder->create_store(gep, factor_alloca);
                        scope.push(factor, factor_alloca);
                    }
                }
            }else{
                factor = node.id;
            }
        }else if((GlobalVariable *)scope.find("@global-int-array" + node.id)){
            auto array = (GlobalVariable *)scope.find("@global-int-array" + node.id);
            gep = builder->create_gep(array, {ConstantInt::get(0, module.get()),ConstantInt::get(0, module.get())});
            factor = "@factor-array-int";
            if(scope.find(factor)){
                auto factor_alloca = (AllocaInst *)scope.find(factor);
                builder->create_store(gep, factor_alloca);
            }else{
                // 这里可能有问题，是不是只用考虑指针类型呢
                //std::cout<<" int-create "<<std::endl;
                auto factor_alloca = builder->create_alloca(TyInt32Ptr);
                builder->create_store(gep, factor_alloca);
                scope.push(factor, factor_alloca);
            }
            
        }else if((GlobalVariable *)scope.find("@global-float-array" + node.id)){
            auto array = (GlobalVariable *)scope.find("@global-int-array" + node.id);
            gep = builder->create_gep(array, {ConstantInt::get(0, module.get()),ConstantInt::get(0, module.get())});
            factor = "@factor-array-float";
            if(scope.find(factor)){
                auto factor_alloca = (AllocaInst *)scope.find(factor);
                builder->create_store(gep, factor_alloca);
            }else{
                // 这里可能有问题，是不是只用考虑指针类型呢
                //std::cout<<" int-create "<<std::endl;
                auto factor_alloca = builder->create_alloca(TyFloatPtr);
                builder->create_store(gep, factor_alloca);
                scope.push(factor, factor_alloca);
            }
        }else if((GlobalVariable *)scope.find("@global-int" + node.id)){
            factor = "@factor-int";
            auto global_addr = (GlobalVariable *)scope.find("@global-int" + node.id);
            if(scope.find(factor)){
                auto factor_alloca = (AllocaInst *)scope.find(factor);
                auto global_load = builder->create_load(global_addr);
                builder->create_store(global_load, factor_alloca);
            }else{
                auto factor_alloca = builder->create_alloca(TyInt32);
                auto global_load = builder->create_load(global_addr);
                builder->create_store(global_load, factor_alloca);
                scope.push(factor, factor_alloca);
            }
        }else if((GlobalVariable *)scope.find("@global-float" + node.id)){
            factor = "@factor-float";
            auto global_addr = (GlobalVariable *)scope.find("@global-float" + node.id);
            if(scope.find(factor)){
                auto factor_alloca = (AllocaInst *)scope.find(factor);
                auto global_load = builder->create_load(global_addr);
                builder->create_store(global_load, factor_alloca);
            }else{
                auto factor_alloca = builder->create_alloca(TyFloat);
                auto global_load = builder->create_load(global_addr);
                builder->create_store(global_load, factor_alloca);
                scope.push(factor, factor_alloca);
            }
        }
        // 通过scope查询
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) { 
    // 判断是数组还是简单变量
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    // 访问express -> var = expression 右边的expression 结点，得到值
    node.expression->accept(*this);
    auto exp_alloca = (AllocaInst*)scope.find(expression);
    auto exp_type = exp_alloca->get_alloca_type();
    auto exp_var = builder->create_load(exp_alloca);
    if(node.var->expression){
        // 数组还是指针？
        // 获取数组的地址
        //std::cout<<" id "<< node.var->id <<std::endl;
        auto alloca_find = (AllocaInst*)scope.find(node.var->id);
        LoadInst *alloca_load;
        Type *alloca_Type;
        if(alloca_find){
            alloca_load = builder->create_load(alloca_find);
            alloca_Type = alloca_find->get_alloca_type();
        }
        // 访问express结点得到偏移量
        node.var->expression->accept(*this);
        // 从expression得到值
        auto offset_Alloca = (AllocaInst *)scope.find(expression);
        auto offset_Alloca_Type = offset_Alloca->get_alloca_type();
        auto offset_load = builder->create_load(offset_Alloca);
        FpToSiInst *offset;
        if(offset_Alloca_Type->is_float_type()){
            // 需要类型转换
            offset = builder->create_fptosi(offset_load, TyInt32);
            
        }else{
            offset = (FpToSiInst *)offset_load;
            //std::cout<<offset<<std::endl;
        }
        // 检查下标是不是大于等于0
        auto bbtrue = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
        auto bbfalse = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
        auto icmp = builder->create_icmp_ge(offset, ConstantInt::get(0, module.get()));
        builder->create_cond_br(icmp, bbtrue, bbfalse);
        builder->set_insert_point(bbfalse);
        auto neg_idx_except_fun = scope.find("neg_idx_except");
        builder->create_call(neg_idx_except_fun, {});
        if(builder->get_insert_block()->get_parent()->get_return_type()->is_integer_type()){
            builder->create_ret(ConstantInt::get(0, module.get()));
        }
        else if(builder->get_insert_block()->get_parent()->get_return_type()->is_float_type()){
            builder->create_ret(ConstantFP::get(0,module.get()));
        }
        else builder->create_void_ret();
        builder->set_insert_point(bbtrue);
        // 如果是数组
        GetElementPtrInst *gep;
        // 是否为全局变量
        if(alloca_find){
            if(alloca_Type->is_array_type()){
                // std::cout<<" array "<< alloca_Type->is_array_type() <<std::endl;
                gep = builder->create_gep(alloca_find, {ConstantInt::get(0, module.get()),offset});
            }else if(alloca_Type->is_pointer_type()){
                // std::cout<<" pointer "<< alloca_find->get_type()->get_pointer_element_type()->is_pointer_type()<<std::endl;
                gep = builder->create_gep(alloca_load, {offset});
                // std::cout<<" pointer "<< alloca_Type->is_pointer_type() <<std::endl;
            }else{
                //std::cout << "error" << std::endl;// error
            }
        }else if((GlobalVariable *)scope.find("@global-int-array" + node.var->id)){
            // 类型为int的全局数组
            auto address = (GlobalVariable *)scope.find("@global-int-array" + node.var->id);
            gep = builder->create_gep(address, {ConstantInt::get(0, module.get()),offset});
        }else if((GlobalVariable *)scope.find("@global-float-array" + node.var->id)){
            auto address = (GlobalVariable *)scope.find("@global-float-array" + node.var->id);
            gep = builder->create_gep(address, {ConstantInt::get(0, module.get()),offset});
        }
        // 检查是否需要进行类型转换
        if(gep->get_element_type()->is_integer_type()){
            expression = "@assign-expression-int";
            // 可能是factor结点调用expression!!!!!!
            factor = expression;
            //   // // // 
            FpToSiInst *var;
            if(exp_type->is_integer_type()){
                // 不需要转换
                builder->create_store(exp_var, gep);
                var = (FpToSiInst *)exp_var;
            }else if(exp_type->is_float_type()){
                auto exp_var_trans = builder->create_fptosi(exp_var, TyInt32);
                builder->create_store(exp_var_trans, gep);
                var = exp_var_trans;
            }
            // 先查询是否使用过
            if(scope.find(expression)){
                // 如果创建过
                auto alloca1 = (AllocaInst *)scope.find(expression);
                // 存储
                builder->create_store(var, alloca1);
            }else{
                // 没有创建
                auto alloca1 = builder->create_alloca(TyInt32);
                scope.push(expression, alloca1);
                builder->create_store(var, alloca1);
            }
        }else if(gep->get_element_type()->is_float_type()){
            expression = "@assign-expression-float";
            // 可能是factor结点调用expression!!!!!!
            factor = expression;
            //   // // // 
            SiToFpInst *var;
            if(exp_type->is_integer_type()){
                auto exp_var_trans = builder->create_sitofp(exp_var, TyFloat);
                builder->create_store(exp_var_trans, gep);
                var = exp_var_trans;
            }else if(exp_type->is_float_type()){
                builder->create_store(exp_var, gep);
                var = (SiToFpInst *)exp_var;
            }
            // 先查询是否使用过
            if(scope.find(expression)){
                // 如果创建过
                auto alloca1 = (AllocaInst *)scope.find(expression);
                // 存储
                builder->create_store(var, alloca1);
            }else{
                // 没有创建
                auto alloca1 = builder->create_alloca(TyFloat);
                scope.push(expression, alloca1);
                builder->create_store(var, alloca1);
            }
        }
        
    }else{
        // 如果是简单变量
        auto alloca1 = (AllocaInst*)scope.find(node.var->id);
        // // 访问express -> var = expression 右边的expression 结点，得到值
        // node.expression->accept(*this);
        // auto exp_alloca = (AllocaInst*)scope.find(expression);
        // auto exp_type = exp_alloca->get_alloca_type();
        // auto exp_var = builder->create_load(exp_alloca);
        // 检查是否需要进行类型转换
        if(alloca1){
            auto alloca_Type = alloca1->get_alloca_type();
            if(alloca_Type->is_integer_type()){
                expression = "@assign-expression-int";
                // 可能是factor结点调用expression!!!!!!
                factor = expression;
                //   // // // 
                FpToSiInst *var;
                if(exp_type->is_integer_type()){
                    // 不需要转换
                    builder->create_store(exp_var, alloca1);
                    var = (FpToSiInst*)exp_var;
                }else if(exp_type->is_float_type()){
                    auto exp_var_trans = builder->create_fptosi(exp_var, TyInt32);
                    builder->create_store(exp_var_trans, alloca1);
                    var = exp_var_trans;
                }
                // 先查询是否使用过
                if(scope.find(expression)){
                    // 如果创建过
                    auto alloca1 = (AllocaInst *)scope.find(expression);
                    // 存储
                    builder->create_store(var, alloca1);
                }else{
                    // 没有创建
                    auto alloca1 = builder->create_alloca(TyInt32);
                    scope.push(expression, alloca1);
                    builder->create_store(var, alloca1);
                }
            }else if(alloca_Type->is_float_type()){
                expression = "@assign-expression-float";
                // 可能是factor结点调用expression!!!!!!
                factor = expression;
                //   // // // 
                SiToFpInst *var;
                if(exp_type->is_integer_type()){
                    auto exp_var_trans = builder->create_sitofp(exp_var, TyFloat);
                    builder->create_store(exp_var_trans, alloca1);
                    var = exp_var_trans;
                }else if(exp_type->is_float_type()){
                    builder->create_store(exp_var, alloca1);
                    var = (SiToFpInst *)exp_var;
                }
                // 先查询是否使用过
                if(scope.find(expression)){
                    // 如果创建过
                    auto alloca1 = (AllocaInst *)scope.find(expression);
                    // 存储
                    builder->create_store(var, alloca1);
                }else{
                    // 没有创建
                    auto alloca1 = builder->create_alloca(TyFloat);
                    scope.push(expression, alloca1);
                    builder->create_store(var, alloca1);
                }
            }
        }else{
            // 全局变量
            if((GlobalVariable *)scope.find("@global-int" + node.var->id)){
                // int
                expression = "@assign-expression-int";
                // 可能是factor结点调用expression!!!!!!
                factor = expression;
                //   // // // 
                auto val = (GlobalVariable *)scope.find("@global-int" + node.var->id);
                FpToSiInst *var;
                // 是否需要类型转换
                if(exp_type->is_integer_type()){
                    // 不需要转换
                    builder->create_store(exp_var, val);
                    var = (FpToSiInst*)exp_var;
                }else if(exp_type->is_float_type()){
                    auto exp_var_trans = builder->create_fptosi(exp_var, TyInt32);
                    builder->create_store(exp_var_trans, val);
                    var = exp_var_trans;
                }
                // 先查询是否使用过
                if(scope.find(expression)){
                    // 如果创建过
                    auto alloca1 = (AllocaInst *)scope.find(expression);
                    // 存储
                    builder->create_store(var, alloca1);
                }else{
                    // 没有创建
                    auto alloca1 = builder->create_alloca(TyInt32);
                    scope.push(expression, alloca1);
                    builder->create_store(var, alloca1);
                }
            }else if((GlobalVariable *)scope.find("@global-float" + node.var->id)){
                auto val = (GlobalVariable *)scope.find("@global-float" + node.var->id);
                expression = "@assign-expression-float";
                // 可能是factor结点调用expression!!!!!!
                factor = expression;
                //   // // // 
                SiToFpInst *var;
                if(exp_type->is_integer_type()){
                    auto exp_var_trans = builder->create_sitofp(exp_var, TyFloat);
                    builder->create_store(exp_var_trans, val);
                    var = exp_var_trans;
                }else if(exp_type->is_float_type()){
                    builder->create_store(exp_var, val);
                    var = (SiToFpInst *)exp_var;
                }
                // 先查询是否使用过
                if(scope.find(expression)){
                    // 如果创建过
                    auto alloca1 = (AllocaInst *)scope.find(expression);
                    // 存储
                    builder->create_store(var, alloca1);
                }else{
                    // 没有创建
                    auto alloca1 = builder->create_alloca(TyFloat);
                    scope.push(expression, alloca1);
                    builder->create_store(var, alloca1);
                }
            }
        }
    }
}

void CminusfBuilder::visit(ASTSimpleExpression &node) { 
    auto TyInt1 = Type::get_int1_type(module.get());
    if(node.additive_expression_r == nullptr){
        // 只有additive-expression
        node.additive_expression_l->accept(*this);
        expression = additive_expression;
    }else{
        auto TyInt32 = Type::get_int32_type(module.get());
        auto TyFloat = Type::get_float_type(module.get());
        node.additive_expression_l->accept(*this);
        std::string expl = additive_expression;
        auto opl_alloca = (AllocaInst*)scope.find(expl);
        auto opl = builder->create_load(opl_alloca);
        auto Ty_alloca_l = opl_alloca->get_alloca_type();
        node.additive_expression_r->accept(*this);
        std::string expr = additive_expression;
       
        auto opr_alloca = (AllocaInst*)scope.find(expr);
        auto opr = builder->create_load(opr_alloca);
        auto Ty_alloca_r = opr_alloca->get_alloca_type();
         // 分别获取两个操作数
        if(Ty_alloca_l->is_integer_type() && Ty_alloca_r->is_integer_type()){
            CmpInst *is_true;
            // 如果都是整型，不需要转换
            if (node.op == OP_LE){
                // 小于或等于
                is_true = builder->create_icmp_le(opl, opr);
            }else if(node.op == OP_LT){
                // 小于
                is_true = builder->create_icmp_lt(opl, opr);
            }else if(node.op == OP_GT){
                is_true = builder->create_icmp_gt(opl, opr);
            }else if(node.op == OP_LE){
                is_true = builder->create_icmp_ge(opl, opr);
            }else if(node.op == OP_EQ){
                is_true = builder->create_icmp_eq(opl, opr);
            }else{
                // 不等于
                is_true = builder->create_icmp_ne(opl, opr);
            }
            auto IsTrue = builder->create_zext(is_true,TyInt32);
            // 将is_true存储
            // 名字怎么取，可不可以重复。----- 每个名字对应一个地址，可以为这种类型的表达式创建一个键值对
            // 每个键值对有两种类型，整型和浮点型，方便创建整型或浮点型的地址
            // 名字与局部变量的名字不能重复，所以应该用特殊符号
            // 规定用@这个特殊符号
            expression = "@simple-expression-int-32";
            // 可能是factor结点调用expression!!!!!!
            //   // // // 
            // 先查询是否使用过
            if(scope.find(expression)){
                // 如果创建过
                auto alloca32 = (AllocaInst *)scope.find(expression);
                // 存储
                builder->create_store(IsTrue, alloca32);
            }else{
                // 没有创建
                auto alloca32 = builder->create_alloca(TyInt32);
                scope.push(expression, alloca32);
                builder->create_store(IsTrue, alloca32);
            }
        }else{
            FCmpInst *is_true;
            SiToFpInst *opl_f, *opr_f;
            // 需要转换
            if(Ty_alloca_l->is_integer_type()){
                // 左边是int，右边是float，将左边转换
                opl_f = builder->create_sitofp(opl, TyFloat);
                opr_f = (SiToFpInst *)opr;
            }else if(Ty_alloca_r->is_integer_type()){
                // 左边是float，右边是int
                opl_f = (SiToFpInst *)opl;
                opr_f = builder->create_sitofp(opr, TyFloat);
            }else{
                opl_f = (SiToFpInst *)opl;
                opr_f = (SiToFpInst *)opr;
            }
            if (node.op == OP_LE){
                // 小于或等于
                is_true = builder->create_fcmp_le(opl_f, opr_f);
            }else if(node.op == OP_LT){
                // 小于
                is_true = builder->create_fcmp_lt(opl_f, opr_f);
            }else if(node.op == OP_GT){
                is_true = builder->create_fcmp_gt(opl_f, opr_f);
            }else if(node.op == OP_LE){
                is_true = builder->create_fcmp_ge(opl_f, opr_f);
            }else if(node.op == OP_EQ){
                is_true = builder->create_fcmp_eq(opl_f, opr_f);
            }else{
                // 不等于
                is_true = builder->create_fcmp_ne(opl_f, opr_f);
            }
            // 将is_true存储
            // 名字怎么取，可不可以重复
            auto IsTrue = builder->create_zext(is_true,TyInt32);
            expression = "@simple-expression-int-32";
            // 先查询是否使用过
            if(scope.find(expression)){
                // 如果创建过
                auto alloca32 = (AllocaInst *)scope.find(expression);
                // 存储
                builder->create_store(IsTrue, alloca32);
            }else{
                // 没有创建
                auto alloca32 = builder->create_alloca(TyInt32);
                builder->create_store(IsTrue, alloca32);
                scope.push(expression, alloca32);
            }
        }
        
    }
    // 可能是factor结点调用expression!!!!!!
    //   // // // 
    factor = expression;
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) { 
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    // additive-expression -> additive-expression addop term
    if(node.additive_expression){
        node.additive_expression->accept(*this);
        auto addexp_alloca = (AllocaInst *)scope.find(additive_expression);
        auto addexp_val = builder->create_load(addexp_alloca);
        auto addexp_type = addexp_alloca->get_alloca_type();
        node.term->accept(*this);
        auto term_alloca = (AllocaInst *)scope.find(term);
        auto term_type = term_alloca->get_alloca_type();
        
        auto term_val = builder->create_load(term_alloca);
        if(addexp_type->is_integer_type() && term_type->is_integer_type()){
            BinaryInst *result;
            // 不需要 类型转换
            if(node.op == OP_PLUS){
                result = builder->create_iadd(addexp_val, term_val);
            }else{
                result = builder->create_isub(addexp_val, term_val);
            }
            additive_expression = "@additive-expression-int";
            if(scope.find(additive_expression)){
                // 如果已经创建了
                auto alloca_find = (AllocaInst *)scope.find(additive_expression);
                builder->create_store(result, alloca_find);
            }else{
                auto alloca_find = builder->create_alloca(TyInt32);
                builder->create_store(result, alloca_find);
                scope.push(additive_expression, alloca_find);
            }
        }else{
            // 两个操作数类型不一样
            SiToFpInst *opl_f, *opr_f;
            BinaryInst *result;
            if(addexp_type->is_integer_type()){
                // 左边是整型，右边是浮点型
                auto addexp_val_trans = builder->create_sitofp(addexp_val, TyFloat);
                opl_f = addexp_val_trans;
                opr_f = (SiToFpInst *)term_val;
            }else if(term_type->is_integer_type()){
                // 左边浮点
                opl_f = (SiToFpInst *)addexp_val;
                opr_f = builder->create_sitofp(term_val, TyFloat);
            }else{
                opl_f = (SiToFpInst *)addexp_val;
                opr_f = (SiToFpInst *)term_val;
            }
            if(node.op == OP_PLUS){
                result = builder->create_fadd(opl_f, opr_f);
            }else{
                result = builder->create_fsub(opl_f, opr_f);
            }
            additive_expression = "@additive-expression-float";
            if(scope.find(additive_expression)){
                auto alloca_find = (AllocaInst *)scope.find(additive_expression);
                builder->create_store(result, alloca_find);
            }else{
                auto alloca_new = builder->create_alloca(TyFloat);
                builder->create_store(result, alloca_new);
                scope.push(additive_expression, alloca_new);
            }
        }
    }else{
        node.term->accept(*this);
        additive_expression = term;
    }
}

void CminusfBuilder::visit(ASTTerm &node) { 
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    if(node.term){
        node.term->accept(*this);
        auto term_alloca = (AllocaInst *)scope.find(term);
        auto term_type = term_alloca->get_alloca_type();
        auto term_val = builder->create_load(term_alloca);
        node.factor->accept(*this);
        auto factor_alloca = (AllocaInst *)scope.find(factor);
        auto factor_type = factor_alloca->get_alloca_type();
        auto factor_val = builder->create_load(factor_alloca);
        if(term_type->is_integer_type() && factor_type->is_integer_type()){
            BinaryInst *result;
            // 不需要 类型转换
            if(node.op == OP_MUL){
                result = builder->create_imul(term_val, factor_val);
            }else{
                result = builder->create_isdiv(term_val, factor_val);
            }
            term = "@term-int";
            if(scope.find(term)){
                // 如果已经创建了
                auto alloca_find = (AllocaInst *)scope.find(term);
                builder->create_store(result, alloca_find);
            }else{
                auto alloca_find = builder->create_alloca(TyInt32);
                builder->create_store(result, alloca_find);
                scope.push(term, alloca_find);
            }
        }else{
            // 两个操作数类型不一样
            SiToFpInst *opl_f, *opr_f;
            BinaryInst *result;
            if(term_type->is_integer_type()){
                // 左边是整型，右边是浮点型
                auto term_val_trans = builder->create_sitofp(term_val, TyFloat);
                opl_f = term_val_trans;
                opr_f = (SiToFpInst *)factor_val;
            }else if(factor_type->is_integer_type()){
                // 左边浮点
                opl_f = (SiToFpInst *)term_val;
                opr_f = builder->create_sitofp(factor_val, TyFloat);
            }else{
                opl_f = (SiToFpInst *)term_val;
                opr_f = (SiToFpInst *)factor_val;
            }
            if(node.op == OP_MUL){
                result = builder->create_fmul(opl_f, opr_f);
            }else{
                result = builder->create_fdiv(opl_f, opr_f);
            }
            term = "@term-float";
            if(scope.find(term)){
                auto alloca_find = (AllocaInst *)scope.find(term);
                builder->create_store(result, alloca_find);
            }else{
                auto alloca_new = builder->create_alloca(TyFloat);
                builder->create_store(result, alloca_new);
                scope.push(term, alloca_new);
            }
        }
    }else{
        node.factor->accept(*this);
        // 可以在遍历的时候给factor赋值，因为var前面并没有用到，可以在var里直接给factor赋值，把factor看成它的上级
        // num一样
        // expression? 递归？可不可以直接在expression给factor赋值
        term = factor;
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    auto id = node.id;
    auto args = node.args;
    auto Fun = (Function *)scope.find(id);
    std::vector<Value *> args_val;
    int i = 0;
    for(auto arg = args.begin(); arg != args.end(); arg++,i++){
        (*arg)->accept(*this);   
        auto arg_alloca = (AllocaInst *)scope.find(expression);
        auto alloca_Type = arg_alloca->get_alloca_type();
        if(alloca_Type->is_array_type()){
                auto arg_load = builder->create_load(arg_alloca);
                args_val.push_back(arg_load);
        }
        else if(alloca_Type->is_float_type())
        {
            auto para_type = Fun->get_function_type()->get_param_type(i);
            if(para_type->is_integer_type()){
                auto arg_load = builder->create_load(arg_alloca);
                auto change = builder->create_fptosi(arg_load, TyInt32);
                args_val.push_back(change);
            }
            else{
                auto arg_load = builder->create_load(arg_alloca);
                args_val.push_back(arg_load);
            }
        }
        else if(alloca_Type->is_integer_type()){
            auto para_type = Fun->get_function_type()->get_param_type(i);
            if(para_type->is_float_type()){
                auto arg_load = builder->create_load(arg_alloca);
                auto change = builder->create_sitofp(arg_load, TyFloat);
                args_val.push_back(change);
            }
            
            else{
                auto arg_load = builder->create_load(arg_alloca);
                
                args_val.push_back(arg_load);
            }
        }
        else{
            auto arg_load = builder->create_load(arg_alloca);
            args_val.push_back(arg_load);
        }
    }
    //std::cout<<" id "<<id<<std::endl;
    if(scope.find(id)){
        auto Fun = (Function *)scope.find(id);
        if(Fun->get_return_type()->is_void_type()){
            builder->create_call(scope.find(id), args_val);
        }else if(Fun->get_return_type()->is_integer_type()){
            auto ret_val = builder->create_call(Fun, args_val);
            factor = "@factor-int";
            // 因为前面的return 已经考虑过了类型转换，所以这里就不必再考虑
            if(scope.find(factor)){
                auto  alloca_rt = scope.find(factor);
                builder->create_store(ret_val, alloca_rt);
            }else{
                auto alloca_rt = builder->create_alloca(Type::get_int32_type(module.get()));
                builder->create_store(ret_val, alloca_rt);
                scope.push(factor, alloca_rt);
            }
        }else{
            auto ret_val = builder->create_call(Fun, args_val);
            factor = "@factor-float";
            // 因为前面的return 已经考虑过了类型转换，所以这里就不必再考虑
            if(scope.find(factor)){
                auto  alloca_rt = scope.find(factor);
                builder->create_store(ret_val, alloca_rt);
            }else{
                auto alloca_rt = builder->create_alloca(Type::get_float_type(module.get()));
                builder->create_store(ret_val, alloca_rt);
                scope.push(factor, alloca_rt);
            }
        }
    }
    
 }

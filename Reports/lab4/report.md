# Lab4 实验报告

小组成员 

柳文浩 PB18000359

朱秋成 PB18000383

黄鸿 PB18000346

## 实验要求

通过访问者模式，利用函数重载的特性，结合IR接口，完成访问者对象的访问函数，使得访问者在访问生成的语法树时，对每个结点，或者生成指令，或者访问它的子节点、传递变量。

## 实验难点

实验中遇到哪些挑战

- scope该怎么使用

- 访问者对象在访问一个结点时，如果当前结点的指令的生成需要子结点的访问结果，那么在访问子结点后，如何获取子结点的结果。

- 全局变量与普通变量该如何区分、使用

- 测试样例12出现标号错误，提示label expected to be xxx(与生成的代码标签序号不一致)，经过组员讨论，并查看了issue之后，得知一个基本块中不应该出现多于一个的终止指令，原本的代码在生成if和while循环的基本块时，会在每个基本块结尾加入跳转指令：

  ```c++
  builder->set_insert_point(trueBB);
      node.if_statement->accept(*this);
      
      /*if(!builder->get_insert_block()->get_terminator()){
          builder->create_br(nextBB);
      }*/
      builder->create_br(nextBB);//一开始在if的trueBB基本块中必定生成跳转到nextBB，即if语句结束后的基本块的指令
  ```

  一旦在基本块中出现了其他终止指令（如return，或者嵌套的if和while等），会导致一个基本块中生成多个终止指令产生错误。解决方案如上述代码块中注释部分所示，利用builder->get_insert_block()->get_terminator() API 检查当前基本块中有无终止语句，若没有则生成相应跳转指令，解决冲突同时也优化了代码冗余。

- 关于关系运算的结果的储存和获取问题，一开始设计的是在SimpleExpression中得到了关系运算的结果即以i1的格式存储，但是遇到了许多问题，比如if语句中load得到的判断结果类型为i1，而利用ConstantInt只能得到32位的常数0，用i1和i32比较大小从而确定跳转到哪个分支，可能会产生不正确的结果。此外，若函数的参数类型是int，而传入的参数类型为i1的关系运算结果，也会导致函数无法正常调用（比如output(1 >= 2)）。解决方案为在SimpleExpression中得到i1类型的结果后立即将其零扩展为i32类型再存储，这样程序中所有用到的整形类型都为i32，避免了上述问题

  ```c++
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
              auto IsTrue = builder->create_zext(is_true,TyInt32);//将结果零扩展为i32类型
              expression = "@simple-expression-int-32";
              if(scope.find(expression)){
                  // 如果创建过
                  auto alloca32 = (AllocaInst *)scope.find(expression);
                  // 存储
                  builder->create_store(IsTrue, alloca32);//储存i32类型的比较结果
              }else{
                  // 没有创建
                  auto alloca32 = builder->create_alloca(TyInt32);
                  scope.push(expression, alloca32);
                  builder->create_store(IsTrue, alloca32);
              }
  ```

- 在var -> ID [expression] 中需要检查数组下标是否为负数，在判断下标为负数的基本块里调用neg_idx_except（）函数进行错误处理并返回。一开始没有考虑到可能会在不同类型的函数中调用neg_idx_except，设置的返回类型都为void，如代码段所示

  ```c++
  		builder->set_insert_point(bbfalse);
          auto neg_idx_except_fun = scope.find("neg_idx_except");
          builder->create_call(neg_idx_except_fun, {});
          /*if(builder->get_insert_block()->get_parent()->get_return_type()->is_integer_type()){
              builder->create_ret(ConstantInt::get(0, module.get()));
          }
          else if(builder->get_insert_block()->get_parent()->get_return_type()->is_float_type()){
              builder->create_ret(ConstantFP::get(0,module.get()));
          }
          else */
  		builder->create_void_ret();
  ```

  当在返回类型不是void的函数里调用时，会出现返回类型不匹配的问题，比如下列程序判断数组下标为负的基本块中若使用create_void_ret()，会报错与函数的返回类型不符

  ```c
  int print(int a[]){
      a[0] = a[0] + 1;
      output(a[0]);
      return 1;
  }
  void main(void){
      int a[3];
      a[0] = 3;
      print(a);
  }
  ```

  在代码块中注释掉的部分即为解决方案，根据builder->get_insert_block()->get_parent()->get_return_type()->is_integer_type()判断返回类型后根据类型返回相应值

- 在给函数传array a[]时，因为传入的是数组a的第一个元素的地址，所以是一个指针。所以在压入scope中要注意使用存储指针的变量如下图：

  ```c++
  auto factor_alloca = builder->create_alloca(TyInt32Ptr);
  ```

  ```c++
  auto factor_alloca = builder->create_alloca(TyFloatPtr);
  ```

  这样就意味着，从scope读出出来的是指向指针的指针。在下列cmius代码中就是描述的问题。

  ```c++
  int gcd(int u[]){
      return u[0];
  }
  void main(void){
      int u[2];
      int d;
      d = gcd(u);
  }
  ```

  u就是传入的地址。

- 在本次实验中所有赋值都要考虑到类型转换，其实这个只是要考虑的范围比较多，但是实现还是比较容易的。

  

## 实验设计

请写明为了顺利完成本次实验，加入了哪些亮点设计，并对这些设计进行解释。
可能的阐述方向有:

1. 如何设计全局变量

   首先要判断一个变量是全局声明还是局部变量

   ```cpp
   if(bb){
           auto fun = bb->get_parent();
           if (fun) {
               xxxx
           }
       xxxxx
   }else{
       xxxx
   }
   ```

   如上述代码，bb块里，fun里的肯定是局部变量，因为全局作用域中并没有bb块以及fun。else的就是全局变量。

   ```cpp
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
   }
   ```

   - 全局变量int型的处理方法如上，通过node.ty判断是属于那种类型的变量。

   - 再根据node是否有num这个子结点，判断是普通变量还是数组类型。
   - 如果是普通变量，那么创建一个全局变量，生成指令。再将其存储到scope里。为了与局部变量名容易区别，这种类型的全局变量在变量名前加@global-int。
   - 如果是数组，那么根据num实际的元素个数创建一个全局数组，再将其地址存储在scope中，方便以后在scope查找。当然，名字也要加一串特殊前缀。

2. 如何使用scope

   ```cpp
   scope为作用域对象，在类中，有一个私有属性inner。
   std::vector<std::map<std::string, Value *>> inner;
   ```

   可以看到，inner是一个vector容器，容器存的还是容器。对于inner[i]，它是一个带键值对的map容器，键为字符串，值为Value *的变量。

   再观察find函数，可以得知它是从容器顶部到底部访问的，所以不需要担心优先使用局部变量的问题。

   我们在进入一个compound-stmt的时候，会调用scope.enter，在退出时会调用scope.exit()。保证了作用域的正确。

   在声明一个变量时，都会为变量分配空间，并把它的空间加入到当前作用域，保证了可以多次改变该变量的值。代码实例如下。

   ```cpp
   if (node.num == nullptr) {
       // 如果不是数组
       auto val_alloca = builder->create_alloca(Ty_Int);
       cope.push(node.id, val_alloca);
   }else if(node.num->i_val > 0){
       // 如果是数组，定义数组类型
       auto ArrayTy = ArrayType::get(Ty_Int, node.num->i_val);
       // 为数组分配空间
       auto array_alloca = builder->create_alloca(ArrayTy);
       scope.push(node.id, array_alloca);
   }else{
       // error
   }
   
   ```

   在使用时，如果通过ID[i]的形式获取值，则ID是一个数组或者指针，方式如下。

   ```cpp
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
   ```

   会从scope找到地址，通过gep指令获得要访问的值的地址，再从该地址获得值并将其存储在特定的地址中，传回父节点。如18行，factor是scope一个作用域里的键，用于存储返回的值。

   如果是通过ID等方式获取值，也与上面类似。

3. 怎么获取子结点的结果

   ```cpp
   std::string expression= "";
   std::string simple_expression = "";
   std::string additive_expression = "";
   std::string term = "";
   std::string factor = "";
   ```

   定义一系列键，字符串为scope里的名字。访问子结点，在子结点中完成对“返回值“的存储。在父节点中直接使用。

   ```cpp
   node.expression->accept(*this);
   auto exp_alloca = (AllocaInst*)scope.find(expression);
   ```

   如上述代码。在' void *CminusfBuilder*::*visit*(*ASTAssignExpression* &node) '访问子结点后，直接通过字符串expression在scope中搜索。搜索到的值是期望得到的值。

   而在子结点中，完成对expression的处理，保证程序的正确。当然，有时候要递归访问。不过不用担心，可以在子结点访问结束时，对expression进行处理。这样，最后返回时，expression是期望得到的。对于sim_expression等键，也是类似的。

   ```cpp
   // 先查询是否使用过
   expression = "@assign-expression-int";
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
   ```

   其本质是创建一个相对类型的地址，用于存储“返回值”。如果之前创建过了，那么就存进去，否则就要创建空间。为了避免与程序的变量冲突，使用@、-组成的字符串加以区分。

4. ASTCompoundStmt的设计：

   ```c++
   	scope.enter();
       for (auto local_dec : node.local_declarations){
           local_dec->accept(*this);
       }
       for (auto statement : node.statement_list){
           statement->accept(*this);
       }
       scope.exit();
   ```

   compound-stmt→{ local-declarations statement-list}，采用两个for循环依次visit local-declarations 和statement-list即可。由于复合语句的局部声明作用域相较全局声明的作用域更高，所以要进入一个新的scope

5. ASTExpressionStmt的设计：

   ```c++
   if(node.expression != nullptr){
            node.expression->accept(*this);
        }
   ```

   $\text{expression-stmt} \rightarrow \text{expression}\ \textbf{;}\ $ 判断node中是否有expression，根据结果visit expression即可

6. ASTFunDeclaration的设计：

   ```
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
   ```

   这一部分对遍历函数的参数列表，得到参数的类型并将参数分别push到params容器里，然后根据node的信息得到函数的返回类型（此时已经完成了params函数的功能，所以params函数不需实现）

   ```c++
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
       node.compound_stmt->accept(*this);//visit compoundstmt
   ```

   这一部分基本思路如注释，处理函数声明的一些细节问题

   ```c++
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
   ```

   当返回类型是void时，判断基本块有没有终止语句（要保证基本块有且只有一个终止语句）

7. ASTIterationStmt的设计：

   ```c
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
   ```

   iteration-stmt→while ( expression ) statement，首先进入beginBB基本块，visit expression获得判断条件，根据获得的不同类型（这里有类型转换），判断若expression为真则跳转到statementBB，否则跳转到endBB，在statementBB中需要判断该基本块中是否已经包含了终止语句来决定是否在最后加入无条件跳转，以保证有且只有一个终止语句，最后当expression为假时跳转到endBB

8. ASTNum的设计：

   ```c
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
   ```

   首先判断node的类型为int || float，利用全局变量factor判断是否已经创建，若未创建则申请相应变量，储存值并加入到scope中，否则只需更新该变量的值

9. ASTReturnStmt的设计：

   $$\text{return-stmt} \rightarrow \textbf{return}\ \textbf{;}\ |\ \textbf{return}\ \text{expression}\ \textbf{;}$$ 

   ```c
   void CminusfBuilder::visit(ASTReturnStmt &node) {
       //std::cout<<"return"<<std::endl;
       auto TyInt32 = Type::get_int32_type(module.get());
       auto TyFloat = Type::get_float_type(module.get());
       
       // 获取函数、函数返回值类型
       auto fun = builder->get_insert_block()->get_parent();
       auto ret_type = fun->get_return_type();
       
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
   ```

   首先得到需要return的函数的返回值类型，然后判断，当需要return一个语句，且函数返回类型不是void时，visit  expression 得到需要返回的值，与函数声明的返回值类型进行比较，并进行合适的类型转换。否则以void形式返回。
   
10. ASTSelectionStmt的设计：

    selection-stmt→ if ( expression ) statement∣ if ( expression ) statement else statement

    ```c
    	node.expression->accept(*this);
        // 从expression获得判断条件
        auto cmp_alloca = (AllocaInst *)scope.find(expression);
        auto cmp_load = builder->create_load(cmp_alloca);
        
        auto trueBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
        BasicBlock *falseBB;
        auto nextBB = BasicBlock::create(module.get(), "", builder->get_insert_block()->get_parent());
    ```

    首先visit expression获得判断条件的值（已经转化为i32类型），提前声明好几个基本块

    ```c
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
    ```

    判断node.else_stmt是否存在，来决定属于哪个产生式。若else存在，则首先考虑expression的类型（这里的判断条件可能是float类型的表达式），进行合适的类型转换，然后创建分支，当条件为真时跳转到trueBB，否则跳转到falseBB。若没有else语句，类似地类型转换后，若条件为真跳转到trueBB，否则跳转到nextBB（即if语句结束）

    ```c
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
    ```

    在trueBB分支中visit node的if_stmt，然后判断trueBB中是否有结束语句，若没有则加入无条件跳转到nextBB的语句（如前所述，这样做是为了防止出现多个终结语句）。对于falseBB的处理也是如此。在if语句的结束插入nextBB进入下一语句

11. ASTSimpleExpression的设计：

    $$\text{simple-expression} \rightarrow \text{additive-expression}\ \text{relop}\ \text{additive-expression}\ |\ \text{additive-expression}$$

    $$relop →<= ∣ < ∣ > ∣ >= ∣ == ∣ !=$$

    $$\text{additive-expression} \rightarrow \text{additive-expression}\ \text{addop}\ \text{term}\ |\ \text{term}$$

    $$addop→+ ∣ -$$

    $$\text{term} \rightarrow \text{term}\ \text{mulop}\ \text{factor}\ |\ \text{factor}$$

    $$mulop→* ∣ / $$

    ```c
    if(node.additive_expression_r == nullptr){
            // 只有additive-expression
            node.additive_expression_l->accept(*this);
            expression = additive_expression;
        }
    ```

    如果产生式右侧只有additive exp，则visit 这个节点，并更新expression

    否则：

    ```c
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
             // 分别visit左右表达式，获取两个操作数
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
                }//以上首先判断两个操作数是不是整形，根据op的类型得到判断结果
         		auto IsTrue = builder->create_zext(is_true,TyInt32);
                //将判断结果零扩展为32位
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
          else{
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
            }//以上是两个操作数类型不同，需要类型转换的情形，其余情况与不需要类型转换类似
            
        }
        // 可能是factor结点调用expression
        
        factor = expression;
            
    ```

    思路如注释所示

12. ASTTerm设计：

    term→term mulop factor ∣ factor

    首先讨论当node.term存在，即为第一个产生式时：

    ```c
    	if(node.term){//判断为第一个产生式
            node.term->accept(*this);
            auto term_alloca = (AllocaInst *)scope.find(term);
            auto term_type = term_alloca->get_alloca_type();
            auto term_val = builder->create_load(term_alloca);
            node.factor->accept(*this);
            auto factor_alloca = (AllocaInst *)scope.find(factor);
            auto factor_type = factor_alloca->get_alloca_type();
            auto factor_val = builder->create_load(factor_alloca);
            /*分别visit term和factor节点，获得他们的类型和value*/
    ```

    然后考虑是否需要类型转换，并执行op的操作得到结果

    ```c
    	if(term_type->is_integer_type() && factor_type->is_integer_type()){
                BinaryInst *result;
                // 不需要 类型转换
                if(node.op == OP_MUL){
                    result = builder->create_imul(term_val, factor_val);
                }else{
                    result = builder->create_isdiv(term_val, factor_val);
                }//执行乘法或除法
                term = "@term-int";
                if(scope.find(term)){
                    // 如果已经创建了
                    auto alloca_find = (AllocaInst *)scope.find(term);
                    builder->create_store(result, alloca_find);
                }else{
                    auto alloca_find = builder->create_alloca(TyInt32);
                    builder->create_store(result, alloca_find);
                    scope.push(term, alloca_find);
                }//store并加入scope
            }
    ```

    对于需要类型转换的情形，判断是左边还是右边需要转换，然后与上述方式一样处理即可。

    对于只有factor，即第二个产生式的情况：

    ```c
    		node.factor->accept(*this);
            
            term = factor;
    ```

    visit factor节点，得到值传递给term即可
    
    
    
13. ASTcall的设计:

    $`\text{call} \rightarrow \textbf{ID}\ \textbf{(}\ \text{args} \textbf{)}`$

    使用访问者模式，从args中读取到要传入函数的元素的指针。然后根据调用函数的形参类型，对实参进行类型转换。

    下列是当传入实参属于数组和浮点型类型时的代码，如果传入的实参时浮点型，而函数形参的类型为int型时，就会发生类型转换。

    ```c++
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
    ```

    如果调用的函数是由返回值的，则接受该函数的返回值。

    ```c++
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
    ```

    

14. ASTVarDeclaration的定义:

    $`\text{var-declaration}\ \rightarrow \text{type-specifier}\ \textbf{ID}\ \textbf{;}\ |\ \text{type-specifier}\ \textbf{ID}\ \textbf{[}\ \textbf{INTEGER}\ \textbf{]}\ \textbf{;}`$

    $`\text{type-specifier} \rightarrow \textbf{int}\ |\ \textbf{float}\ |\ \textbf{void}`$

    变量的声明主要分为全局变量和局部变量的声明，在变量中也分为数组，浮点，整型，void变量的声明。在我们的代码中主要通过判断是否在块中，判断声明的是全局变量还是局部变量。在bb中声明就为局部变量，不在bb中声明的就是全局变量。并且是根据变量名创建对应的scope然后分配指针，插入的对应的scope中。

    ​	下图代码为局部变量数组的声明，其他变量相似。

    ```c++
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
    ```

    ​	下图代码为全局变量数组的声明，其他变量相似。（全局变量的具体处理方法在前面有涉及到）

    ```c++
    else {
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
    ```

    

15. ASTVar的设计：

    $`\text{var} \rightarrow \textbf{ID}\ |\ \textbf{ID}\ \textbf{[}\ \text{expression} \textbf{]}`$

    ASTVar的主要作用是根据变量名，返回存储该变量数据的指针，当然该变量可以是a[i], array a, int a, float a，还可以是指针，所以得分情况考虑。

    当node.expression存在时，说明该var是具有偏移量的，所以取值是要进行create_gep,下列为其获取值的代码。

    ```c++
    if(builder->get_insert_block()->get_parent()->get_return_type()->is_integer_type()){
                builder->create_ret(ConstantInt::get(0, module.get()));
            }
            else if(builder->get_insert_block()->get_parent()->get_return_type()->is_float_type()){
                builder->create_ret(ConstantFP::get(0,module.get()));
            }
    ```

    最后是将值压入到scope中存储。

    ```c++
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
    ```

    当node.expression不存在时，则变量的类型可以是数组的首地址和指针还有普通变量，所以会调用下面语句进行判断。

    ```c++
    if(alloca_Type->is_array_type()){
        ...
    }
    else if(alloca_Type->is_pointer_type()){
         ...
    }
    else{
      ...  
    }
    ```

    下列代码为Var是数组头地址的读取方法。

    ```c++
    if(alloca_Type->is_array_type()){
                    gep = builder->create_gep(alloca1, {ConstantInt::get(0, module.get()),ConstantInt::get(0, module.get())});
                    if(gep->get_element_type()->is_integer_type()){
                        factor = "@factor-array-int";
                        if(scope.find(factor)){
                            auto factor_alloca = (AllocaInst *)scope.find(factor);
                            builder->create_store(gep, factor_alloca);
                        }else{
                            // 这里可能有问题，是不是只用考虑指针类型呢
                            std::cout<<" int-create "<<std::endl;
                            auto factor_alloca = builder->create_alloca(TyInt32Ptr);
                            builder->create_store(gep, factor_alloca);
                            scope.push(factor, factor_alloca);
                        }
                    }
    ```

    

16. ASTAssignExpression的定义:

    $`\text{expression} \rightarrow \text{var}\ \textbf{=}\ \text{expression}\ |\ \text{simple-expression}`$

    $`\text{var} \rightarrow \textbf{ID}\ |\ \textbf{ID}\ \textbf{[}\ \text{expression} \textbf{]}`$

    总体思路就是，利用访问者模式调用var，返回指向指向它的指针，然后再调用它的expression，返回要赋得值。最后将值赋值给var.

    因为前期没有考虑号ASTVar的接口，导致在该部分的代码中完成了部分ASTVar的功能，有些冗余。

    当然在赋值过程中考虑到了强制类型转换，数组下标越界等问题，所以整个代码会过长，所欲i下列仅展示int型变量的赋值过程（也就是压入scope的过程)

    ```c++
    if(exp_type->is_integer_type()){
                        auto exp_var_trans = builder->create_sitofp(exp_var, TyFloat);
                        builder->create_store(exp_var_trans, val);
                        var = exp_var_trans;
                    }else if(exp_type->is_float_type()){
                        builder->create_store(exp_var, val);
                        var = (SiToFpInst *)exp_var;
                    }
    ```

    

17. ASTAdditiveExpression的定义:

    $`\text{additive-expression} \rightarrow \text{additive-expression}\ \text{addop}\ \text{term}\ |\ \text{term}`$

    $`\text{addop} \rightarrow \textbf{+}\ |\ \textbf{-}`$

    有上式可知这个是一个左递归文法，所以先使用访问者模式递归调用node.additive_expression，然后再与term进行运算。最后将得到的值压入scope中。唯一值得注意的是，其中也要进行强制类型转换。

    下列为无强制类型转换的代码:

    ```c++
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
    ```

    下图为有强制类型转换的代码:

    ```C++
    else{
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
    ```

    

    

    



### 实验总结

通过本次实验，我们扩展了lab3部分的工作，实现了一个可以编译任意合法的Cminus-f程序产生llvm中间代码的简易编译器。通过本次实验，我们学到了很多新知识，有了很多收获：

- 熟练掌握了C++语言的很多知识和技巧，提高了编程能力
- 积累了大型项目开发的经验，对于分离式编译、头文件的书写、运用脚本判断结果正误等技巧也有了更多了解
- 较为深入的了解了编译器实现的细节，对于C语言的许多特性，如变量的作用域、函数的参数传递、自动类型转换、内存分配等了解了背后的机理
- 扩展了Git的用法
- 积累了团队协作开发的经验，尝试了并行工作提高效率
- 熟练使用llvmIR的访问者模式设计算法，遍历语法树



### 实验反馈 （可选 不会评分）

对本次实验的建议

- 希望随实验给出的测试样例中能涵盖更多情况
- 希望实验文档可以写的更加详细，便于在实验初期找到思路

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
































# Lab5 实验报告

队长姓名:柳文浩

队长学号:PB18000359

队员1姓名:黄鸿

队员1学号:PB18000346

队员2姓名:朱秋成

队员2学号:PB18000383

## 实验要求

请按照自己的理解，写明本次实验需要干什么

在lab4的基础上，完成常量传播，循环不变式外提和活跃变量分析三个优化pass，实现简单的编译器优化

- 常量传播

  对每一条表达式，判断它是否能计算出结果。如果能计算出结果，则计算出结果，则用计算的结果代替这条表达式被引用的地方，随后将这条指令删除。对于全局变量来说，判断在一个块里，是否存在store语句且该语句将值存到全局变量的地址，如果存在，那么对于这条语句后面的所有指令（同一个块），如果用到了存储的值，直接用值代替load指令。

  在有必要时，删除冗余分支。

- 循环外提

  主要是将循环中用不到的指令外移，且在该循环中，外移的指令的结果不变。用以减少循环的开销。

- 活跃变量分析

  分析bb块入口和出口处的活跃变量并输出

## 实验难点

1. 常量传播

   没什么难的，修改phi 指令难一点吧，需要了解phi指令操作数的构成。除此之外，彻底删除冗余块的递归过程也有点坑。

2. 循环外提

   同样没什么难度，了解操作数如果不是常量就可以转换成指令，掌握获取循环入口块、获取前驱与后继块的方法，那完成这一部分没什么难度。要明白哪些指令不能外提，指令外提的条件是什么。

3. 活跃变量分析

   针对phi指令对方程的修改，算法转换成代码实现



## 实验设计

* 常量传播
    实现思路：
    
    1.  参照返回整型值的常量折叠实现，实现了返回浮点值的常量折叠实现，基本思路相同，首先从两个参数中得到相应的浮点值，然后根据运算符op的类型，进行相应的运算得到结果并返回。
    
       代码：
    
       ```c++
       ConstantFP *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2) {
           float c_value1 = value1->get_value();
           float c_value2 = value2->get_value();
           switch (op)
           {
           case Instruction::fadd:
               return ConstantFP::get(c_value1 + c_value2, module_);
       
               break;
           case Instruction::fsub:
               return ConstantFP::get(c_value1 - c_value2, module_);
               break;
           case Instruction::fmul:
               return ConstantFP::get(c_value1 * c_value2, module_);
               break;
           case Instruction::fdiv:
               return ConstantFP::get((float)(c_value1 / c_value2), module_);
               break;
           default:
               return nullptr;
               break;
           }
       }
       ```
    
    2. 运用dynamic_cast判断value是不是整型/浮点型常量的同时返回相应的整型/浮点常量值
    
       dynamic_cast：只有类型转换合法才会返回转换值，否则返回nullptr，可以用来判定value类型
    
       代码：
    
       ```c++
       // 用来判断value是否为ConstantFP，如果不是则会返回nullptr
       ConstantFP *cast_constantfp(Value *value)
       {
           auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
           if (constant_fp_ptr)
           {
               return constant_fp_ptr;
           }
           else
           {
               return nullptr;
           }
       }
       ConstantInt *cast_constantint(Value *value)
       {
           auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
           if (constant_int_ptr)
           {
               return constant_int_ptr;
           }
           else
           {
               return nullptr;
           }
       }
       ```
    
    3. delete_bb_and_succ函数删除基本块bb和其他当bb被删除后失去前驱的冗余块
    
       ```c++
       // 从块的列表中删去一个块，并删除其它冗余块
       void delete_bb_and_succ(BasicBlock *bb) {
           // 获取待删除的块的所有后继
           auto succ_bbs = bb->get_succ_basic_blocks();
           // 删除该块后，对原来所有后继，如果没有前驱，那么它们是冗余块，需要递归删除
           bb->erase_from_parent();
           auto instruction_list = bb->get_instructions();
           for (auto instruction : instruction_list) {
               bb->delete_instr(instruction);
           }
           for (auto succ_bb : succ_bbs) {
               if (succ_bb->get_pre_basic_blocks().empty()) {
                   delete_bb_and_succ(succ_bb);
               }
           }
       }
       
       ```
    
    4. 通过bb块中instruction数量判断是否已经被删除
    
       ```c++
       // 用以判断一个块是否已经被删除
       bool has_deleted_bb(BasicBlock *bb) {
           if (bb->get_instructions().empty()) {
               return true;
           }
           return false;
       }
       ```
    
    5. 首先通过函数列表遍历所有的函数，遍历每个函数的bb块，遍历每个bb块的指令列表，根据指令的类型，用不同方法判断能否常量传播。
    
       ```c++
                       for (auto instruction : instructions) {//遍历每一条指令
                           if (instruction->isBinary()) {//判断指令的类型
                               auto val1 = instruction->get_operand(0);
                               auto val2 = instruction->get_operand(1);
                               /*判断指令的两个操作数是不是相同类型的常量*/
                               if (cast_constantfp(val1) && cast_constantfp(val2)) {
                                   auto new_val = folder.compute(instruction->get_instr_type(), (ConstantFP *) val1, (ConstantFP *) val2);
                                   instruction->replace_all_use_with(new_val);//修改使用者的操作数
                                   instruction->remove_use_of_ops();
                                   cur_bb->delete_instr(instruction);//删除指令和所有产生的冗余
                                   has_change = true;
                               } else if (cast_constantint(val1) && cast_constantint(val2)) {
                                   auto new_val = folder.compute(instruction->get_instr_type(), (ConstantInt *) val1, (ConstantInt *) val2);
                                   instruction->replace_all_use_with(new_val);
                                   instruction->remove_use_of_ops();
                                   cur_bb->delete_instr(instruction);
                                   has_change = true;
                               }
                           }
       ```
    
       ```c++
       				else if (instruction->is_si2fp()) {
                               if (cast_constantint(instruction->get_operand(0))) {
                                   // 获取未转换前的整数
                                   auto val = ((ConstantInt *)instruction->get_operand(0))->get_value();
                                   // 修改使用者用到的值
                                   instruction->replace_all_use_with(ConstantFP::get(float(val), m_));
                                   // 从该条指令操作数的使用者列表移除
                                   instruction->remove_use_of_ops();
                                   // 删除该条指令
                                   cur_bb->delete_instr(instruction);
                                   has_change = true;
                               }
                           }else if (instruction->is_fp2si()) {
                               if (cast_constantfp(instruction->get_operand(0))) {
                                   // 获取操作数
                                   auto val = ((ConstantFP *) instruction->get_operand(0))->get_value();
                                   // 修改所有使用者的操作数
                                   instruction->replace_all_use_with(ConstantInt::get((int)val, m_));
                                   // 从该条指令操作数的使用者列表移除
                                   instruction->remove_use_of_ops();
                                   // 删除该指令
                                   cur_bb->delete_instr(instruction);
                                   has_change = true;
                               }
                           } else if (instruction->is_zext()) {
                               if (cast_constantint(instruction->get_operand(0))) {
                                   //获取操作数
                                   auto val = (ConstantInt *)(instruction->get_operand(0));
                                   auto new_val = ConstantInt::get(val->get_value(), m_);
                                   instruction->replace_all_use_with(new_val);//修改使用者的操作数
                                   instruction->remove_use_of_ops();// 从该条指令操作数的使用者列表移除
                                   cur_bb->delete_instr(instruction);//删除指令
                                   has_change = true;
                               }
                           } 
       ```
    
    对于cmp指令，如果两个操作数都是常数，则根据cmp的op，计算出比较结果，用比较的结果代替原来的cmp指令。
    
    
    
       ```c++
       					else if (instruction->is_cmp()) {
                               auto val1 = cast_constantint(instruction->get_operand(0));
                               auto val2 = cast_constantint(instruction->get_operand(1));
                               if (val1 && val2) {
                                   // 两个操作数都是常数
                                   auto cmpinstruction = (CmpInst *)instruction;
                                   // 折叠该指令
                                   bool cmp;
                                   switch (cmpinstruction->get_cmp_op())
                                   {
                                   case CmpInst::EQ:
                                       cmp = val1->get_value() == val2->get_value();
                                       break;
                                   case CmpInst::NE:
                                       cmp = val1->get_value() != val2->get_value();
                                       break;
                                   case CmpInst::GT:
                                       cmp = val1->get_value() > val2->get_value();
                                       break;
                                   case CmpInst::GE:
                                       cmp = val1->get_value() >= val2->get_value();
                                       break;
                                   case CmpInst::LT:
                                       cmp = val1->get_value() < val2->get_value();
                                       break;
                                   case CmpInst::LE:
                                       cmp = val1->get_value() <= val2->get_value();
                                       break;
                                   default:
                                       cmp = false;
                                       break;
                                   }
                                   // 通过比较的结果获取常数
                                   auto new_val = ConstantInt::get(cmp, m_);
                                   // 修改所有使用者的操作数
                                   instruction->replace_all_use_with(new_val);
                                   instruction->remove_use_of_ops();
                                   cur_bb->delete_instr(instruction);
                                   has_change = true;
                               }
                           }else if (instruction->is_fcmp()) {
                            auto val1 = cast_constantfp(instruction->get_operand(0));
                            auto val2 = cast_constantfp(instruction->get_operand(1));
                            if (val1 && val2) {
                                // 两个操作数都是常数
                                auto fcmpinstruction = (FCmpInst *)instruction;
                                // 折叠该指令
                                bool fcmp;
                                switch (fcmpinstruction->get_cmp_op())
                                {
                                case FCmpInst::EQ:
                                    fcmp = val1->get_value() == val2->get_value();
                                    break;
                                case FCmpInst::NE:
                                    fcmp = val1->get_value() != val2->get_value();
                                    break;
                                case FCmpInst::GT:
                                    fcmp = val1->get_value() > val2->get_value();
                                    break;
                                case FCmpInst::GE:
                                    fcmp = val1->get_value() >= val2->get_value();
                                    break;
                                case FCmpInst::LT:
                                    fcmp = val1->get_value() < val2->get_value();
                                    break;
                                case FCmpInst::LE:
                                    fcmp = val1->get_value() <= val2->get_value();
                                    break;
                                default:
                                    fcmp = false;
                                    break;
                                }
                                // 通过比较的结果获取常数
                                auto new_val = ConstantInt::get(fcmp, m_);
                                // 修改所有使用者的操作数
                                instruction->replace_all_use_with(new_val);
                                // std::cout << "okjkkk" << cmp << std::endl;
                                instruction->remove_use_of_ops();
                                cur_bb->delete_instr(instruction);
                                has_change = true;
                            }
                        } 
       ```
    
       对于br指令，首先判断br的条件是不是常数，若为常数，则根据条件的真假删除掉这条指令，加入新的无条件跳转指令到相应的基本块，并删除无用的基本块  
    
    ```c++
    				else if (instruction->is_br()) {
                            // 通过操作数的个数，寻找条件跳转语句，再判断条件是否为常数
                            if (instruction->get_num_operand() == 3) {
                                // std::cout << ((ConstantInt *)instruction->get_operand(0))->get_value() << std::endl;
                                auto cond = instruction->get_operand(0);
                                if (cast_constantint(cond)) {
                                    // 条件为常数
                                    auto val = ((ConstantInt *) cond)->get_value();
                                    auto bb_true = (BasicBlock *)instruction->get_operand(1);
                                    auto bb_false = (BasicBlock *)instruction->get_operand(2);
                                    if (val == 0) {
                                        // false
                                        // 删除指令，加入无条件跳转，删除true的基本块
                                        cur_bb->delete_instr(instruction);
                                        auto builder = new IRBuilder(cur_bb, m_);
                                        builder->create_br(bb_false);
                                        if (bb_true->get_use_list().empty()) {
                                            delete_bb_and_succ(bb_true);
                                        }
                                    } else {
                                        // true
                                        // 删除指令，加入无条件跳转，删除false基本块
                                        cur_bb->delete_instr(instruction);
                                        auto builder = new IRBuilder(cur_bb, m_);
                                        builder->create_br(bb_true);
                                        if (bb_false->get_use_list().empty()) {
                                            delete_bb_and_succ(bb_false);
                                        }
                                    }
                                    has_change = true;
                                }
                            }
                        }
    ```
    
    对于phi指令，首先获取其长度，然后两个一组分别处理，如果某组的基本块已经被删除，则去掉这一组对应的指令，如果此时phi指令已经为空，则删除该phi指令。处理完所有组后，如果操作数正好为2，则将其替换成普通的赋值语句。
    
    ```c++
    					else if (instruction->is_phi()) {
                            // std::cout << instruction->print() << std::endl;
                            int i = 0;
                            int num = instruction->get_num_operand();
                            while (i < num / 2) {
                                auto bb = (BasicBlock *)instruction->get_operand(2 * i + 1);
                                if (has_deleted_bb(bb)) {
                                    if (num == 2) {
                                        // 删去
                                        instruction->remove_use_of_ops();
                                        cur_bb->delete_instr(instruction);
                                        num = 0;
                                    } else {
                                        instruction->remove_operands(2 * i, 2 * i + 1);
                                        num = instruction->get_num_operand();
                                    }
                                } else {
                                    i++;
                                }
                            }
                            if (num == 2) {
                                auto new_val = instruction->get_operand(0);
                                instruction->replace_all_use_with(new_val);
                                instruction->remove_use_of_ops();
                                cur_bb->delete_instr(instruction);
                                has_change = true;
                            }
                        }
    ```
    
    对于load指令，如果在该指令所在的基本块中，之前有对同一个地址的store指令，则可以将load替换为store指令的操作数。首先遍历load指令所在的基本块，若达到该load指令则循环退出，因为之后的指令无法优化该指令。若有存入地址与load地址相同的指令则记录，以得到最近的符合条件的store指令。
    
    ```c++
    				else if (instruction->is_load()) {
                            // 如果是load指令
                            // 在本块、该指令前有store对地址赋值，则无需load
                            auto load_addr = instruction->get_operand(0);
                            bool should_optimization = false;
                            Instruction *optimization_instruction;
                            for (auto cur_bb_instruction : cur_bb->get_instructions()) {
                                if (cur_bb_instruction == instruction) {
                                    break;
                                } else {
                                    if (cur_bb_instruction->is_store()) {
                                        // 如果为store指令，查看存入的地址是否对应当前load指令的地址
                                        // 获取存入的地址，查看是否对应
                                        if (cur_bb_instruction->get_operand(1) == load_addr) {
                                            should_optimization = true;
                                            optimization_instruction = cur_bb_instruction;
                                        } else {
                                            continue;
                                        }
                                    }
                                }
                            }
                            if (should_optimization) {
                                // 如果可以进行常量传播，不必再load
                                instruction->replace_all_use_with(optimization_instruction->get_operand(0));
                                cur_bb->delete_instr(instruction);
                                has_change = true;
                            }
                        } 
    ```
    
    6. 循环执行上述代码，每轮循环更新函数的bb块列表，直到某一轮循环无法做出新的优化停止。
    
    
    
    优化前后的IR对比（举一个例子）并辅以简单说明：
    
    对于常量传播的testcase1：
    
    ```c
    void main(void){
        int i;
        int idx;
    
        i = 0;
        idx = 0;
    
        while(i < 100000000)
        {
            idx = 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 ;
            i=i+idx*idx*idx*idx*idx*idx*idx*idx/(idx*idx*idx*idx*idx*idx*idx*idx);
        }
    	output(idx*idx);
        return ;
    }
    ```
    
    优化前产生的IR为
    
    ```
    define void @main() {
    label_entry:
      br label %label2
    label2:                                                ; preds = %label_entry, %label7
      %op78 = phi i32 [ 0, %label_entry ], [ %op73, %label7 ]
      %op79 = phi i32 [ 0, %label_entry ], [ %op40, %label7 ]
      %op4 = icmp slt i32 %op78, 100000000
      %op5 = zext i1 %op4 to i32
      %op6 = icmp ne i32 %op5, 0
      br i1 %op6, label %label7, label %label74
    label7:                                                ; preds = %label2
      %op8 = add i32 1, 1
      %op9 = add i32 %op8, 1
      %op10 = add i32 %op9, 1
      %op11 = add i32 %op10, 1
      %op12 = add i32 %op11, 1
      %op13 = add i32 %op12, 1
      %op14 = add i32 %op13, 1
      %op15 = add i32 %op14, 1
      %op16 = add i32 %op15, 1
      %op17 = add i32 %op16, 1
      %op18 = add i32 %op17, 1
      %op19 = add i32 %op18, 1
      %op20 = add i32 %op19, 1
      %op21 = add i32 %op20, 1
      %op22 = add i32 %op21, 1
      %op23 = add i32 %op22, 1
      %op24 = add i32 %op23, 1
      %op25 = add i32 %op24, 1
      %op26 = add i32 %op25, 1
      %op27 = add i32 %op26, 1
      %op28 = add i32 %op27, 1
      %op29 = add i32 %op28, 1
      %op30 = add i32 %op29, 1
      %op31 = add i32 %op30, 1
      %op32 = add i32 %op31, 1
      %op33 = add i32 %op32, 1
      %op34 = add i32 %op33, 1
      %op35 = add i32 %op34, 1
      %op36 = add i32 %op35, 1
      %op37 = add i32 %op36, 1
      %op38 = add i32 %op37, 1
      %op39 = add i32 %op38, 1
      %op40 = add i32 %op39, 1
      %op44 = mul i32 %op40, %op40
      %op46 = mul i32 %op44, %op40
      %op48 = mul i32 %op46, %op40
      %op50 = mul i32 %op48, %op40
      %op52 = mul i32 %op50, %op40
      %op54 = mul i32 %op52, %op40
      %op56 = mul i32 %op54, %op40
      %op59 = mul i32 %op40, %op40
      %op61 = mul i32 %op59, %op40
      %op63 = mul i32 %op61, %op40
      %op65 = mul i32 %op63, %op40
      %op67 = mul i32 %op65, %op40
      %op69 = mul i32 %op67, %op40
      %op71 = mul i32 %op69, %op40
      %op72 = sdiv i32 %op56, %op71
      %op73 = add i32 %op78, %op72
      br label %label2
    label74:                                                ; preds = %label2
      %op77 = mul i32 %op79, %op79
      call void @output(i32 %op77)
      ret void
    }
    ```
    
    可以看出优化前每轮循环循环体都执行了所有的加法和乘除，效率极低
    
    优化后产生的IR为
    
    ```
    define void @main() {
    label_entry:
      br label %label2
    label2:                                                ; preds = %label_entry, %label7
      %op78 = phi i32 [ 0, %label_entry ], [ %op73, %label7 ]
      %op79 = phi i32 [ 0, %label_entry ], [ 34, %label7 ]
      %op4 = icmp slt i32 %op78, 100000000
      %op5 = zext i1 %op4 to i32
      %op6 = icmp ne i32 %op5, 0
      br i1 %op6, label %label7, label %label74
    label7:                                                ; preds = %label2
      %op73 = add i32 %op78, 1
      br label %label2
    label74:                                                ; preds = %label2
      %op77 = mul i32 %op79, %op79
      call void @output(i32 %op77)
      ret void
    }
    ```
    
    优化后的IR将idx折叠成34，并将i=i+idx*idx*idx*idx*idx*idx*idx*idx/(idx*idx*idx*idx*idx*idx*idx*idx)直接替换成i = i + 1，
    
    同时常量传播修改了对应的phi指令，并且删除了所有的无用指令，完成了优化要求。
    
    


* 循环不变式外提
    实现思路：
    
    1. none_operands_had_assigned判断某指令的所有操作数在循环中有没有被赋值，如果所有操作数都没有在循环中被赋值，即认为是可以被外提的指令。
    
       ```c++
       bool none_operands_had_assigned(Instruction *instruction, BBset_t *loop) {
           // 如果传入的指令的所有操作数在循环中没有被赋值，则返回true
           bool none_had_assign = true;
           for (auto operand : instruction->get_operands()) {
               auto op_inst = dynamic_cast<Instruction *>(operand);
               if (op_inst) {
                   // std::cout << inst_op->print() << std::endl;
                   auto in_which_bb = op_inst->get_parent();
                   if (loop->find(in_which_bb) != loop->end()) {
                       // 如果指令所在的块在循环内
                       none_had_assign = false;
                       break;
                   }
               }
           }
           return none_had_assign;
       }
       ```
    
    2. 先通过LoopSearch获取循环的相关信息，然后对每个循环的每个基本块的每条指令遍历，若该指令的所有操作数在当前循环中没有被赋值，且指令属于可以外提的类型，则将指令外提至上一轮循环。
    
       循环操作，若某轮循环无法继续优化，则优化结束。
    
       ```c++
       while (has_change) {
               has_change = false;//若本轮进行了优化吗，才将has_change置为1
               for (auto loop = loop_searcher.begin(); loop != loop_searcher.end(); loop++) {
                   for (auto bb : **loop) {
                       auto instructions = bb->get_instructions();
                       for (auto instruction : instructions) {
                           // std::cout << bb->get_name() << "   " << bb->get_num_of_instr() << std::endl;
                           if (!instruction) {
                               // std::cout << "heuohwguoewhg" << std::endl;
                           }
                           // std::cout << instruction->print() << std::endl;
                           // 如果在该循环中，结果不变，即操作数在该循环中没被赋值，或者为常数
                           need_to_hoist = true;
                           auto ins_type = instruction->get_instr_type();
                           std::vector<Instruction::OpID> types {Instruction::phi,Instruction::ret,Instruction::br,Instruction::call};
                           for(auto type : types){
                               if(ins_type == type)need_to_hoist = false;
                           }//检查指令是否为不可外提的类型
                           if (need_to_hoist == true && none_operands_had_assigned(instruction, *loop)) {
                               // 移到
                               
                               auto base_bb = loop_searcher.get_loop_base(*loop);
                               // 将指令移动到循环入口块的不在当前循环内的前驱，即提到外层循环
                               for (auto pre : base_bb->get_pre_basic_blocks()) {
                                   if ((*loop)->find(pre) == (*loop)->end()) {
                                       auto operands = instruction->get_operands();
                                       auto use_list = instruction->get_use_list();
                                       // instruction->remove_operands(0, instruction->get_num_operand() - 1);
                                       std::cout << "delete " << instruction->print() << std::endl;
                                       //在当前bb块中删除该指令
                                       bb->delete_instr(instruction);
                                       has_change = true;
                                       //更新指令列表
                                       instructions = bb->get_instructions();
                                       int i = 0;
                                       for (auto operand : operands) {
                                           instruction->set_operand(i, operand);
                                           i++;
                                       }
                                       // 保存终结指令，删除终结指令，加入新的指令，加入终结指令
                                       auto terminal_instruction = pre->get_terminator();
                                       pre->delete_instr(terminal_instruction);
                                       pre->add_instruction(instruction);
                                       pre->add_instruction(terminal_instruction);
                                       instruction->set_parent(pre);
                                   }
                               }
                           }
                       }
                       
                   }
               }
           }
       ```
    
       
    
    
    优化前后的IR对比（举一个例子）并辅以简单说明：
    
    对于循环不变式外提的testcase1：
    
    ```
    void main(void){
        int i;
        int j;
        int ret;
    
        i = 1;
        
        while(i<10000)
        {
            j = 0;
            while(j<10000)
            {
                ret = (i*i*i*i*i*i*i*i*i*i)/i/i/i/i/i/i/i/i/i/i;
                j=j+1;
            }
            i=i+1;
        }
    	output(ret);
        return ;
    }
    ```
    
    优化前产生的IR为：
    
    ```
    define void @main() {
    label_entry:
      br label %label3
    label3:                                                ; preds = %label_entry, %label58
    	;
      %op61 = phi i32 [ %op64, %label58 ], [ undef, %label_entry ]
      %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58 ]	;i
      %op63 = phi i32 [ %op65, %label58 ], [ undef, %label_entry ]
      %op5 = icmp slt i32 %op62, 10000
      %op6 = zext i1 %op5 to i32
      %op7 = icmp ne i32 %op6, 0
      br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
      br label %label11
    label9:                                                ; preds = %label3
      call void @output(i32 %op61)
      ret void
    label11:                                                ; preds = %label8, %label16
    	;外层循环
      %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
      %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
      %op13 = icmp slt i32 %op65, 10000
      %op14 = zext i1 %op13 to i32
      %op15 = icmp ne i32 %op14, 0
      br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
    	;内层循环
      %op19 = mul i32 %op62, %op62
      %op21 = mul i32 %op19, %op62
      %op23 = mul i32 %op21, %op62
      %op25 = mul i32 %op23, %op62
      %op27 = mul i32 %op25, %op62
      %op29 = mul i32 %op27, %op62
      %op31 = mul i32 %op29, %op62
      %op33 = mul i32 %op31, %op62
      %op35 = mul i32 %op33, %op62
      %op37 = sdiv i32 %op35, %op62
      %op39 = sdiv i32 %op37, %op62
      %op41 = sdiv i32 %op39, %op62
      %op43 = sdiv i32 %op41, %op62
      %op45 = sdiv i32 %op43, %op62
      %op47 = sdiv i32 %op45, %op62
      %op49 = sdiv i32 %op47, %op62
      %op51 = sdiv i32 %op49, %op62
      %op53 = sdiv i32 %op51, %op62
      %op55 = sdiv i32 %op53, %op62
      %op57 = add i32 %op65, 1
      br label %label11
    label58:                                                ; preds = %label11
      %op60 = add i32 %op62, 1
      br label %label3
    }
    ```
    
    可以看出在内层循环中集中运算了很多乘除语句，可以外提到外层循环
    
    优化后的IR为：
    
    ```
    define void @main() {
    label_entry:
      br label %label3
    label3:                                                ; preds = %label_entry, %label58
      %op61 = phi i32 [ %op64, %label58 ], [ undef, %label_entry ]
      %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58 ]
      %op63 = phi i32 [ %op65, %label58 ], [ undef, %label_entry ]
      %op5 = icmp slt i32 %op62, 10000
      %op6 = zext i1 %op5 to i32
      %op7 = icmp ne i32 %op6, 0
      br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
    	;外层循环
      %op19 = mul i32 %op62, %op62
      %op21 = mul i32 %op19, %op62
      %op23 = mul i32 %op21, %op62
      %op25 = mul i32 %op23, %op62
      %op27 = mul i32 %op25, %op62
      %op29 = mul i32 %op27, %op62
      %op31 = mul i32 %op29, %op62
      %op33 = mul i32 %op31, %op62
      %op35 = mul i32 %op33, %op62
      %op37 = sdiv i32 %op35, %op62
      %op39 = sdiv i32 %op37, %op62
      %op41 = sdiv i32 %op39, %op62
      %op43 = sdiv i32 %op41, %op62
      %op45 = sdiv i32 %op43, %op62
      %op47 = sdiv i32 %op45, %op62
      %op49 = sdiv i32 %op47, %op62
      %op51 = sdiv i32 %op49, %op62
      %op53 = sdiv i32 %op51, %op62
      %op55 = sdiv i32 %op53, %op62
      br label %label11
    label9:                                                ; preds = %label3
      call void @output(i32 %op61)
      ret void
    label11:                                                ; preds = %label8, %label16
      %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
      %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
      %op13 = icmp slt i32 %op65, 10000
      %op14 = zext i1 %op13 to i32
      %op15 = icmp ne i32 %op14, 0
      br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
    	;内层循环
      %op57 = add i32 %op65, 1
      br label %label11
    label58:                                                ; preds = %label11
      %op60 = add i32 %op62, 1
      br label %label3
    }
    ```
    
    可以看出内层循环只需要保留j = j + 1即可，将大量运算提前到外层循环。




* 活跃变量分析
    
    
    
    实现思路：
    
    1. ​	再hpp文件中定义每个块的use和def集合
    
       ```c++
       private:
           Function *func_;
           std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
           std::set<Value *> def, use, phi_mid;
       ```
    
       
    
    2. 给每一个块的live_out和live_in初始化为空集
    
       ```c++
       for (auto bb : func->get_basic_blocks()){
        	live_in.insert(std::pair<BasicBlock *, std::set<Value*>>(bb, {}));//置为空集
         	live_out.insert(std::pair<BasicBlock *, std::set<Value*>>(bb, {}));//置为空集
       } 
       ```
    
       
    
    3. 使用bool型变量mark来标记在循环整个bb块后live_in有没有发生改变
    
       ```c++
       bool mark = 1;
       while (mark == 1)
       ```
    
       
    
    4. 先计算块的live_out集合，因为phi的特殊性，所以在计算live_out集合时。先遍历该块的每一个后继块中的phi指令，剔除别的块传入的变量。将这些不需要的变量传入phi_mid变量中，最后在要计算live_out集合时剔除。
    
       ```c++
       for (auto BB : bb->get_succ_basic_blocks()){
           for (auto INST : BB->get_instructions()){
               int i = 0;
               if (INST->is_phi()){
                   for(auto phi_bb : INST->get_operands()){
                       if (phi_bb->get_type()->is_label_type()){
                           auto use_node = INST->get_operand(i-1);
                           if (phi_bb != bb){ //传入phi的bb块和要求live_out的bb块不一致
                               phi_mid.insert(use_node); //其中的变量就不能插入live_out中
                           }
                       }
                       i++;
                   }
               }
               else {
                   break;
               }
           }
           for (auto Val : live_in[BB]){
               if (phi_mid.find(Val) == phi_mid.end())//检查是否有phi_mid变量，其中的变量不能插入live_out
                   live_out[bb].insert(Val);
           }
           phi_mid.clear();
       }   
       ```
    
       
    
    5. 要计算块的live_in集合就要先计算该块中的def和use集合，在循环每条指令时取指令的左值作为use集合，当然要剔除掉已经计算出的def集合和常数变量，bb变量，函数名变量，和void变量。接着再取每个指令的左值作为def变量，当然也要剔除之前已经被调用的变量。
    
       ```c++
       for (auto INST : bb->get_instructions()){
           int i = 0;
           for (auto use_node : INST->get_operands()){
               std:: cout << use_node->print() << std::endl;
               auto op_inst = dynamic_cast<Constant *>(use_node);
               if (op_inst)
                   continue;
               if (use_node->get_type()->is_label_type())//判断bb类型
                   continue;
               if (use_node->get_type()->is_void_type())//判断空类型
                   continue;
               if (use_node->get_type()->is_function_type())//判断是否为函数类型
                   continue;
               if (def.find(use_node) == def.end())//判断是否再def集合中出现
                   use.insert(use_node);
           }
           if (use.find(INST) == use.end()) { //判断是否在use集合中出现
               auto val = (Value *)INST;
               if (val)    //判断是否为空
                   def.insert(val);
           } 
       
       }
       ```
    
       
    
    6. 通过得到的live_out 和得到的def，use变量集合计算出live_in
    
       ```c++
       for (auto VAL : use){
           if (live_in[bb].find(VAL) == live_in[bb].end())
               mark = 1;   //live_in发生改变
           live_in[bb].insert(VAL);
       }
       for (auto VAL : live_out[bb]){
           if (def.find(VAL) != def.end()){
               continue;
           }
           else
           {
               if (live_in[bb].find(VAL) == live_in[bb].end())
                   mark = 1;  //live_in发生改变
               live_in[bb].insert(VAL);
           }
       
       }
       ```
    
       
    
* 实现思路：
    相应的代码：

### 实验总结

此次实验有什么收获

常量传播&&循环不变式外提：深入理解了编译器做优化的具体方式，对各种优化的原理也有了更多了解，编程过程中对各种指令的特点，与源代码的对应，循环的处理等也更加熟练。

活跃变量:

​	编写过程加深了对活跃变量的理解，了解其具体实现过程。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息

#include "ConstPropagation.hpp"
#include "logging.hpp"

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式
ConstantInt *ConstFolder::compute(
    Instruction::OpID op,
    ConstantInt *value1,
    ConstantInt *value2)
{

    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);

        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

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

// 用以判断一个块是否已经被删除
bool has_deleted_bb(BasicBlock *bb) {
    if (bb->get_instructions().empty()) {
        return true;
    }
    return false;
}

void ConstPropagation::run()
{
    // 从这里开始吧！
    //std::cout << "Hello World!" <<std::endl;
    auto funs = this->m_->get_functions();
    bool has_change = true;
    ConstFolder folder(m_);
    while (has_change) {
        has_change = false;
        for (auto fun : funs) {
            auto bbs = fun->get_basic_blocks();
            for (auto cur_bb : bbs) {
                auto instructions = cur_bb->get_instructions();
                for (auto instruction : instructions) {
                    if (instruction->isBinary()) {
                        auto val1 = instruction->get_operand(0);
                        auto val2 = instruction->get_operand(1);
                        if (cast_constantfp(val1) && cast_constantfp(val2)) {
                            auto new_val = folder.compute(instruction->get_instr_type(), (ConstantFP *) val1, (ConstantFP *) val2);
                            instruction->replace_all_use_with(new_val);
                            instruction->remove_use_of_ops();
                            cur_bb->delete_instr(instruction);
                            has_change = true;
                        } else if (cast_constantint(val1) && cast_constantint(val2)) {
                            auto new_val = folder.compute(instruction->get_instr_type(), (ConstantInt *) val1, (ConstantInt *) val2);
                            instruction->replace_all_use_with(new_val);
                            instruction->remove_use_of_ops();
                            cur_bb->delete_instr(instruction);
                            has_change = true;
                        }
                    } else if (instruction->is_si2fp()) {
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
                    } else if (instruction->is_fp2si()) {
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
                            auto val = (ConstantInt *)(instruction->get_operand(0));
                            auto new_val = ConstantInt::get(val->get_value(), m_);
                            instruction->replace_all_use_with(new_val);
                            instruction->remove_use_of_ops();
                            cur_bb->delete_instr(instruction);
                            has_change = true;
                        }
                    } else if (instruction->is_cmp()) {
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
                    } else if (instruction->is_fcmp()) {
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
                    } else if (instruction->is_br()) {
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
                    } else if (instruction->is_phi()) {
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
                    } else if (instruction->is_load()) {
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
                    } else if (instruction->is_store()) {
                        std::cout <<instruction->get_num_operand() << std::endl;
                    }
                    instructions = cur_bb->get_instructions();
                }
                bbs = fun->get_basic_blocks();
            }
        }
    }
}
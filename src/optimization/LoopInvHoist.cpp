#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"



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
void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, true);
    loop_searcher.run();
    // 接下来由你来补充啦！
    bool has_change = true;
    bool need_to_hoist = true;
    while (has_change) {
        has_change = false;
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
                    }
                    if (need_to_hoist == true && none_operands_had_assigned(instruction, *loop)) {
                        // 移到
                        
                        auto base_bb = loop_searcher.get_loop_base(*loop);
                        // 将指令移到基本块的前驱，且前驱不在该循环
                        for (auto pre : base_bb->get_pre_basic_blocks()) {
                            if ((*loop)->find(pre) == (*loop)->end()) {
                                auto operands = instruction->get_operands();
                                auto use_list = instruction->get_use_list();
                                // instruction->remove_operands(0, instruction->get_num_operand() - 1);
                                std::cout << "delete " << instruction->print() << std::endl;
                                bb->delete_instr(instruction);
                                has_change = true;
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
}
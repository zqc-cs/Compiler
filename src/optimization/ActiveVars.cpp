#include "ActiveVars.hpp"

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;
            func_->set_instr_name();
            live_in.clear();
            live_out.clear();

            for (auto bb : func->get_basic_blocks()){
                live_in.insert(std::pair<BasicBlock *, std::set<Value*>>(bb, {}));
                live_out.insert(std::pair<BasicBlock *, std::set<Value*>>(bb, {}));
            }

            bool mark = 1;
            while (mark == 1)
            {
                std :: cout << " 123" << std ::endl;
                mark = 0;
                for (auto bb : func->get_basic_blocks()){
                    for (auto BB : bb->get_succ_basic_blocks()){
                        for (auto INST : BB->get_instructions()){
                            int i = 0;
                            if (INST->is_phi()){
                                for(auto phi_bb : INST->get_operands()){
                                    if (phi_bb->get_type()->is_label_type()){
                                        auto use_node = INST->get_operand(i-1);
                                        if (phi_bb != bb){
                                            phi_mid.insert(use_node);
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
                            if (phi_mid.find(Val) == phi_mid.end())
                                live_out[bb].insert(Val);
                        }
                        phi_mid.clear();
                    }
                    for (auto INST : bb->get_instructions()){
                        int i = 0;
                        /*if (INST->is_phi())
                        {
                            for (auto phi_BB : INST->get_operands()){
                                if (phi_BB->get_type()->is_label_type()){
                                    auto use_node = INST->get_operand(i-1);
                                    auto op_inst = dynamic_cast<Constant *>(use_node);
                                    if (op_inst)
                                        continue;
                                    else
                                    {
                                        if(live_in[phi_BB].find(use_node) != live_in[phi_BB].end())
                                            
                                    }
                                    
                                }
                                i ++;
                            }

                            if (use.find(INST) == use.end()) {
                                auto val = (Value *)INST;
                                def.insert(val);
                            } 
                        }
                        else */
                        {

                            for (auto use_node : INST->get_operands()){
                                std:: cout << use_node->print() << std::endl;
                                auto op_inst = dynamic_cast<Constant *>(use_node);
                                //auto op_gr = dynamic_cast<GlobalVariable *>(use_node);
                                if (op_inst)
                                    continue;
                                /*if (op_gr)
                                    continue;*/
                                if (use_node->get_type()->is_label_type())
                                    continue;
                                if (use_node->get_type()->is_void_type())
                                    continue;
                                if (use_node->get_type()->is_function_type())
                                    continue;
                                if (def.find(use_node) == def.end())
                                    use.insert(use_node);
                            }
                            if (use.find(INST) == use.end()) {
                                auto val = (Value *)INST;
                                def.insert(val);
                            }
                        }
                    }
                    for (auto VAL : use){
                        if (live_in[bb].find(VAL) == live_in[bb].end())
                            mark = 1;
                        live_in[bb].insert(VAL);
                    }
                    for (auto VAL : live_out[bb]){
                        if (def.find(VAL) != def.end()){
                            continue;
                        }
                        else
                        {
                            if (live_in[bb].find(VAL) == live_in[bb].end())
                                mark = 1;
                            live_in[bb].insert(VAL);
                        }

                    }
                    use.clear();
                    def.clear();
                }

                //auto bb = func->get_basic_blocks().back();

            }


            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";

    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}
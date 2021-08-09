# Lab5 实验报告-阶段一

小组成员 

柳文浩 PB18000359

朱秋成 PB18000383

黄鸿 PB18000346

## 实验要求

请按照自己的理解，写明本次实验需要干什么

## 思考题
### LoopSearch
1. 循环的入口如何确定？循环的入口的数量可能超过1嘛？

   在函数find_loop_base中，传入的参数set为当前所在循环的强连通分量，reserved为外层循环中去掉的base节点（对循环的分析从外向内逐层进行，每次分析完一个loop就去掉该loop的base节点，然后重新寻找强连通分量）

   ```c++
   CFGNodePtr LoopSearch::find_loop_base(
       CFGNodePtrSet *set,
       CFGNodePtrSet &reserved)
   {
   
       CFGNodePtr base = nullptr;
       bool hadBeen = false;
       /*首先在set集合中寻找，set代表当前循环的强连通分量。对set中的每个基本块，
       判断其前驱是否也在set中。如果该基本块所有的前驱都在同一个set中，说明该块一定
       不是循环的入口块。否则（即set->find(prev) == set->end()），该基本快至少
       有一个前驱不在set中，说明该块就是当前循环的入口。*/
       /*合法（存在某个前驱不在set中）的基本块可能有多个，该算法找出最后一个符合条件的*/
       for (auto n : *set)
       {
           for (auto prev : n->prevs)
           {
               if (set->find(prev) == set->end())
               {
                   base = n;
               }
           }
       }
       //如果在上一次循环中找到了入口，直接返回，否则从reserved中继续寻找
       /*上面的循环没有找到入口，可能是因为入口块的前驱正好是之前剥离掉的外层循环base块，此时由于内层块
       失去了与base的联系，找不到前驱不在set中的基本块。但是由于reserved中的节点仍然保留与set中的节点
       联系的信息，所以可以从reserved集合中找到*/
       if (base != nullptr)
           return base;
       /*reserved为外层循环中剥离出来的入口块。与上面类似，reserved中的块，若存在
       后继不在reserved中的，就是下一层循环的之前一个节点，就是循环的入口*/
       for (auto res : reserved)
       {
           for (auto succ : res->succs)
           {
               if (set->find(succ) != set->end())
               {
                   base = succ;
               }
           }
       }
   	
       return base;
   }
   ```

   会用到reserved集合寻找入口块的一个例子：首先处理外层循环{1,2,3,4}，入口块为1，然后剥离1，剩余强连通分量{2,3,4}，此时他们的前驱都在集合{2,3,4}中，所以需要从reserved集合{1}中寻找，找到合法的入口块2

   ```mermaid
   graph TB;
   0-->1;1-->2;2-->3;4-->2;3-->4;4-->1;4-->5;
   ```

   

   

   **如果只考虑Cminus语法，循环入口只有1个，因为`while`循环的条件块只能有一个；如果考虑goto语句，循环的入口可能超过1**，比如下面的情况：

   ```mermaid
   graph TB;
   1-->2;
   2-->3;
   3-->4;
   2-->4;
   4-->3;
   ```

   在强连通分量{3,4}中，3和 4都满足存在一个前驱不在该强连通分量中。该算法找到遍历时最后一个符合条件的基本块。

2. 简述一下算法怎么解决循环嵌套的情况。

   对于嵌套的循环，总体思路是从总的强连通分量开始，先寻找最外层的循环的入口，然后在强连通分量中去除其他块与找到的入口块的联系，重新划分强连通分量，相当于剥去外层循环，继续寻找下一层循环的入口，直到强连通分量的大小为0结束。

   ```c++
   while (strongly_connected_components(nodes, sccs))//每次循环重新生成强连通分量
               {
   
                   if (sccs.size() == 0)//如果强连通分量的size为0代表已经没有内层循环了，退出
                   {
                       
                       break;
                   }
                   else
                   {
                       // step 3: find loop base node for each strongly connected graph
   
                       for (auto scc : sccs)//处理每一个强连通分量（循环）
                       {
                           scc_index += 1;
   
                           auto base = find_loop_base(scc, reserved);//寻找本循环的入口
   
                           // step 4: store result进行一系列存储操作维护几个变量以备后用
                           auto bb_set = new BBset_t;
                           std::string node_set_string = "";
   
                           for (auto n : *scc)
                           {
                               bb_set->insert(n->bb);
                               node_set_string = node_set_string + n->bb->get_name() + ',';
                           }
                           loop_set.insert(bb_set);
                           func2loop[func].insert(bb_set);
                           base2loop.insert({base->bb, bb_set});
                           loop2base.insert({bb_set, base->bb});
                           for (auto bb : *bb_set)
                           {
                               if (bb2base.find(bb) == bb2base.end())
                               {
                                   bb2base.insert({bb, base->bb});
                               }
                               else
                               {
                                   bb2base[bb] = base->bb;
                               }
                           }
                           // step 5: map each node to loop base
                           for (auto bb : *bb_set)
                           {
                               if (bb2base.find(bb) == bb2base.end())
                                   bb2base.insert({bb, base->bb});
                               else
                                   bb2base[bb] = base->bb;
                           }
   
                           // step 6: remove loop base node for researching inner loop
                           /*这一步中将上面找到的本次循环的入口变量从cfg中移除，添加到reserved集合
                           中，并将cfg中剩余的块与base块的联系抹除（去除base的前驱和后继与base的单							向关系），保留从base到其前驱和后继的信息，以便find_loop_base函数使用。*/
                           reserved.insert(base);
                           dump_graph(*scc, func->get_name() + '_' + std::to_string(scc_index));
                           nodes.erase(base);
                           for (auto su : base->succs)
                           {
                               su->prevs.erase(base);
                           }
                           for (auto prev : base->prevs)
                           {
                               prev->succs.erase(base);
                           }
   
                       } // for (auto scc : sccs)
                       /*清除信息，方便下一次构造cfg图*/
                       for (auto scc : sccs)
                           delete scc;
                       sccs.clear();
                       for (auto n : nodes)
                       {
                           n->index = n->lowlink = -1;
                           n->onStack = false;
                       }
                   } // else
               } 
   ```

---

### Mem2reg

1. 

   支配边界的定义:

   ​	n的支配边界为：该边界的点是从n可达，但是，是n不支配的第一个结点。（第一个顶点的概念是属于该边界的结点的前驱结点不满足前面说所的条件）

2. 

   ​	phi是，在某个基本块（设该基本块为B0）的支配边界处（支配边界处也是基本块），定义的存储该块的变量临时值的结构。

   ​	phi中存储着。变量出现在各个基本块中出现中的值。（当然该变量表示的是一个地址，这个地址中存储的值在不同的基本块是不一样的）

   ​	如下代码，该变量出现在基本块label10，基本块label_entry。

   `	%op20 = phi i32 [ %op4, %label_entry ], [ %op3, %label10 ]`

3. 

   前后变化

   ​	在没有优化之前，所有基本块中要读取一个变量的值时，都要创建一个地址空间，通过存储进内存再load出来的操作，优化之后，删除了多余的alloca, load, store,操作。可以直接利用phi，使用变量进行操作。例如gcd中label_entry这个基本块在使用arg1的时候，之前是将arg1存进op3再读取出来，优化之后可以直接使用arg1。当然，这个arg1是通过phi获得的值，其他部分同理。

4. 

   在每一个基本块中函数中的某些变量会被重新赋值，设存在一个块B0，其中的地址a（变量）存储的值发生了变化（意味着就要更新phi），它会影响的B0支配的所有块，因为要到达这些块就必须经过B0。所以要通过先根次序遍历支配树，其中对定义和使用都进行重命名。重命名的时候就会将该变量的最新值加入到其phi中，下列代码就是这个操作。

   ```cpp
   if ( instr->is_phi())
               {
                   auto l_val = static_cast<PhiInst *>(instr)->get_lval();
                   if (var_val_stack.find(l_val)!= var_val_stack.end())
                   {                    
                       assert(var_val_stack[l_val].size()!=0);
                       // step 6: fill phi pair parameters
                       static_cast<PhiInst *>(instr)->add_phi_pair_operand( var_val_stack[l_val].back(), bb); //插入变量的最新值
                   }
                   // else phi parameter is [ undef, bb ]
               }
   ```

5. 

   `instr->replace_all_use_with(var_val_stack[l_val].back()); `

   通过维护变量的一个栈来维护变量的最新值，该栈顶元素就是最新值，上述代码就是将该栈的栈顶元素赋值给load变量，维护该栈的方式就是当有store指令时，就不断把读取到的值压栈，所以栈顶元素就是最新值。

---

### 代码阅读总结

#### **loopsearch：**

loopsearch的功能主要是建立CFG图，找到程序中所有的循环和循环的入口块，维护一系列成员变量（如loop2base，bb2base等）

build_cfg  函数将基本块映射成CFG图，同时维护index，onstack等信息方便tarjian算法寻找强连通分量

```c++
void LoopSearch::build_cfg(Function *func, std::unordered_set<CFGNode *> &result)
{
    std::unordered_map<BasicBlock *, CFGNode *> bb2cfg_node;
    for (auto bb : func->get_basic_blocks())
    {
        auto node = new CFGNode;
        node->bb = bb;
        node->index = node->lowlink = -1;
        node->onStack = false;
        bb2cfg_node.insert({bb, node});

        result.insert(node);
    }
    for (auto bb : func->get_basic_blocks())
    {
        auto node = bb2cfg_node[bb];
        std::string succ_string = "success node: ";
        for (auto succ : bb->get_succ_basic_blocks())//建立CFG图的边
        {
            succ_string = succ_string + succ->get_name() + " ";
            node->succs.insert(bb2cfg_node[succ]);
        }
        std::string prev_string = "previous node: ";
        for (auto prev : bb->get_pre_basic_blocks())
        {
            prev_string = prev_string + prev->get_name() + " ";
            node->prevs.insert(bb2cfg_node[prev]);
        }
    }
}
```

strongly_connected_components函数输入CFG图和结果集合，使用Tarjan算法计算出所有强连通分量，将强连通分量存入结果，返回值可以用来判断是否已经不存在强连通分量。

get_parent_loop得到输入loop的外一层的循环，如果没有则返回空。

```c++
BBset_t *LoopSearch::get_parent_loop(BBset_t *loop)
{
    auto base = loop2base[loop];//首先找到loop对应的入口块
    for (auto prev : base->get_pre_basic_blocks())
    {
        if (loop->find(prev) != loop->end())//如果入口块的某个前驱也在loop中，则他不在外层循环
            continue;
        auto loop = get_inner_loop(prev);
        /*if中的第二种情况对应想要找的loop已经是最外层循环，而其base的前驱节点正好在另一个循环中，
        应该返回nullptr*/
        if (loop == nullptr || loop->find(base) == loop->end())
            return nullptr;
        else
        {
            return loop;
        }
    }
    return nullptr;
}
```

---

#### Dominators.cpp

1. ​	该代码维护着ll代码中各个基本块的支配域和直接支配域，以及各个节点的支配边界。它同时还会构造ll代码中支配树。同时会有支配树的遍历方法。

---

#### Men2Reg.cpp

1. 该代码基本上描述着静态单赋值的优化过程

2. 第一步先运行Dominators.cpp，配置好后面要用到的支配边界和支配树。

   ```c++
   dominators_ = new Dominators(m_);
       dominators_->run();
   ```

3. 第二步配置phi

   1. 查找各个基本块中的store指令以确定有多少种存储变量的地址以及他们所出现的基本块。

      ```c++
      var_is_killed.insert(l_val);
      live_var_2blocks[l_val].insert(bb); //得到该变量所出现在的基本块中
      ```

      

   2. 遍历变量出现在的基本块，以及该基本块的支配边界，配置变量phi，并插入在支配边界块的头部

      ```c++
      auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb); 
      ```

      

4. 重命名

   1. 找出该块的phi指令，获取存储变量的地址，往该变量栈种插入phi指令

   2. 遍历指令，如果是load指令，且load的对象是存在phi的，那就load的值替换为栈中的最新push进去的值。并将该指令放入删除列表

      ```c++
      instr->replace_all_use_with(var_val_stack[l_val].back()); //读取该变量的最新值
      wait_delete.push_back(instr); //放进要删除的列表
      ```

   3. 遍历指令，如果是store指令，就将要store的值压入栈，所以栈顶是最新的值。并将该指令放入删除列表

      ```c++
      var_val_stack[l_val].push_back(r_val);  //将要存储的值压栈进入
      wait_delete.push_back(instr); 
      ```

   4. 给phi指令填充栈中最新的参数。否则就是[udf, bb]

      ```c++
      assert(var_val_stack[l_val].size()!=0);// step 6: fill phi pair parameters 
      static_cast<PhiInst *>(instr)->add_phi_pair_operand( var_val_stack[l_val].back(), bb); 
      ```

   5. 遍历支配树

      ```c++
      for ( auto dom_succ_bb : dominators_->get_dom_tree_succ_blocks(bb) )
      {
      	re_name(dom_succ_bb);
      }
      ```

   6. 将当前静态单赋值变量形式名的集合恢复到访问到当前基本块前的状态

   7. 删除指令

      ```c++
      for ( auto instr : wait_delete)
      {
          bb->delete_instr(instr);
      }
      ```

      

5. 删除alloca指令

   

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息



#ifndef __pcodeop_h__
#define __pcodeop_h__

#include "opcodes.hh"
#include "types.h"
#include "varnode.hh"
#include "heritage.hh"
#include <bitset>

#define PCODE_DUMP_VAL              0x01
#define PCODE_DUMP_UD               0x02
#define PCODE_DUMP_DEAD             0x04
/* 有些def的值被use的太多了，可能有几百个，导致整个cfg图非常的不美观，可以开启这个标记，打印cfg时，只打印部分use，
具体多少，可以参考print_udchain的值
*/

#define PCODE_OMIT_MORE_USE         0x08            
#define PCODE_OMIT_MORE_DEF         0x10            
#define PCODE_OMIT_MORE_BUILD       0x20            
#define PCODE_OMIT_MORE_IN          0x40
#define PCODE_HTML_COLOR            0x80

#define PCODE_DUMP_ALL              ~(PCODE_OMIT_MORE_USE | PCODE_OMIT_MORE_DEF | PCODE_OMIT_MORE_BUILD | PCODE_OMIT_MORE_IN)
#define PCODE_DUMP_SIMPLE           0xffffffff

class flowblock;
class varnode;
class funcdata;

class pcodeop_lite {
public:
    OpCode opcode;
    varnode *output = NULL;
    vector<varnode *> inrefs;

    pcodeop_lite(int s);
    ~pcodeop_lite();
};

class pcodeop {
public:
    struct {
        unsigned startblock : 1;
        unsigned branch : 1;
        unsigned call : 1;
        unsigned returns: 1;
        unsigned nocollapse : 1;
        unsigned dead : 1;
        unsigned marker : 1;        // 特殊的站位符， (phi 符号 或者 间接引用 或 CPUI_COPY 对同一个变量的操作)，
        unsigned boolouput : 1;     // 布尔操作

        unsigned coderef : 1;
        unsigned startinst : 1;     // instruction的第一个pcode
        /* 临时算法有用:
        1. compute_sp
        */
        unsigned mark : 1;          // 临时性标记，被某些算法拿过来做临时性处理，处理完都要重新清空

        unsigned branch_call : 1;   // 一般的跳转都是在函数内进行的，但是有些壳的函数，会直接branch到另外一个函数里面去
        unsigned exit : 1;          // 这个指令起结束作用
        unsigned inlined : 1;       // 这个opcode已经被inline过了
        unsigned changed : 1;       // 这个opcode曾经被修改过
        unsigned input : 1;         // input有2种，一种是varnode的input，代表这个寄存器来自于
        unsigned copy_from_phi : 1;           // 给opcode为cpy的节点使用，在删除只有2个入边的join node时，会导致这个节点的phi节点修改成
                                    // copy，这里要标识一下
        unsigned vm_vis : 1;        // 给vm做标记用的
        unsigned vm_eip : 1;
        unsigned zero_load : 1;         // 0地址访问
        unsigned force_constant : 1;    // 强制常量，用来在某些地方硬编码时，不方便计算，人工计算后，强行填入
        unsigned trace : 1;
            /* 
            FIXME:
            会影响load行为的有2种opcode
            1. store
            2. sp = sp - xxx
            后面一种opcode，我们假设alloc出的内存空间值都是0，这个是有问题的?
            */
        unsigned val_from_sp_alloc : 1;     // 这个load的值并非来自于store，而是来自于sp的内存分配行为
		unsigned uncalculated_store : 1;	// 这个store节点是不可计算的
        unsigned itblock : 1;
        unsigned mark_cond_copy_prop: 1;    

        unsigned store_on_dead_path : 1;
    } flags = { 0 };

    OpCode opcode;
    /* 一个指令对应于多个pcode，这个是用来表示inst的 */
    SeqNum start;
    flowblock *parent;
    /* 我们认为程序在分析的时候，sp的值是可以静态分析的，他表示的并不是sp寄存器，而是系统当前堆栈的深度 */
    int     sp = 0;
    Address *disaddr = NULL;

    varnode *output = NULL;
    vector<varnode *> inrefs;

    funcdata *callfd = NULL;   // 当opcode为call指令时，调用的

    bitset<32>      live_in;
    bitset<32>      live_out;
    bitset<32>      live_gen;
    bitset<32>      live_kill;
    /* 数据写入obj时的文件位置 */
    int             ind = 0;

    /* block 里的 */
    list<pcodeop *>::iterator basiciter;
    /* deadlist 里的 */
    list<pcodeop *>::iterator insertiter;
    /* 特殊操作类型里的，比如 store ,load, call */
    list<pcodeop *>::iterator codeiter;

    /* sideeffect_ops中的迭代器 */
    list<pcodeop *>::iterator sideiter;
    list<pcodeop *> mayuses;

    pcodeop(int s, const SeqNum &sq);
    pcodeop(pcodeop_lite *lite);
    ~pcodeop();

    void            set_opcode(OpCode op);
    varnode*        get_in(int slot) { return inrefs[slot];  }
    varnode*        get_in(const Address &addr) {
        for (int i = 0; i < inrefs.size(); i++) {
            if (inrefs[i]->get_addr() == addr) return inrefs[i];
        }

        return NULL;
    }
    varnode*        get_out() { return output;  }
    const Address&  get_addr() { return start.getAddr();  }
    /* dissasembly 时用到的地址 */
    const Address&  get_dis_addr(void) { return disaddr ? disaddr[0] : get_addr(); }

    int             num_input() { return inrefs.size();  }
    void            clear_input(int slot);
    void            remove_input(int slot);
    void            insert_input(int slot);

    void            set_input(varnode *vn, int slot) { inrefs[slot] = vn; }
    int             get_slot(const varnode *vn) { 
        int i, n; n = inrefs.size();
        for (i = 0; i < n; i++)
            if (inrefs[i] == vn) return i;
        return -1;
    }
    int             dump(char *buf, uint32_t flags);
    /* trace compute 
    这个compute_t 和传统的compute不一样，普通的comupte只能用来做常量传播，这个compute_t
    是在循环展开时，沿某条路径开始计算

    compute_t 和 模拟执行是完全不一样的， 模拟执行会给所有系统寄存器赋值，然后走入函数，
    compute_t 不会做这样，compute_t是在某条路径上执行，假如可以计算，就计算，假如不能计算
    就跳过

@inslot         常量传播时，填-1，假如在做trace分析时，填从哪个快进入
@return     
            1       unknown bcond
    */

    /* 碰见了可以计算出的跳转地址 */
#define         ERR_MEET_CALC_BRANCH            1
#define         ERR_UNPROCESSED_ADDR            2
#define         ERR_CONST_CBRANCH               4
#define         ERR_FREE_SELF                   8
    /* 

    branch:         计算的时候发现可以跳转的地址
    wlist:          工作表，当我们跟新某些节点的时候，发现另外一些节点也需要跟新，就把它加入到这个链表内
    */
    int             compute(int inslot, flowblock **branch);
	int				compute_add_sub();
    void            set_output(varnode *vn) { output = vn;  }
    void            set_top() { if (output) output->type.height = a_top; }
    void            set_top_odd() { if (output) output->type.height = a_top_odd; }
    void            set_top_even() { if (output) output->type.height = a_top_even;  }

    bool            is_dead(void) { return flags.dead;  }
    bool            have_virtualnode(void) { return inrefs.size() == 3;  }
    varnode*        get_virtualnode(void) { return inrefs.size() == 3 ? inrefs[2]:NULL;  }
    bool            is_call(void) { return (opcode == CPUI_CALL) || (opcode == CPUI_CALLIND) || callfd; }
    bool            is_coderef(void) { return flags.coderef; }
    void            set_input() { flags.input = 1;  }
    intb            get_call_offset() { return get_in(0)->get_addr().getOffset(); }
    /* 当自己的结果值为output时，把自己整个转换成copy形式的constant */
    void            to_constant(void);
    void            to_constant1(void);
    void            to_rel_constant(void);
    void            to_copy(varnode *in);
    /* 转换成nop指令 */
    void            to_nop(void);
    void            add_mayuse(pcodeop *p) { mayuses.push_back(p);  }
    bool            is_trace() { return flags.trace;  }
    void            set_trace() { flags.trace = 1; }
    void            clear_trace() { flags.trace = 0;  }
    /* in的地址是否在sp alloc内存的位置 */
    bool            in_sp_alloc_range(varnode *in);

    void            on_MULTIEQUAL();
    /*
    识别以下pattern(里面的数可以替换):

    v7 = y_72;
    if ( y_72 > 9 )
      v6 = 1;
    v8 = 1467139458;
    if ( y_72 > 9 )
      v8 = -486874641;
    if ( v6 )
      v8 = 1467139458;


    if ( v8 == 1467139458 ) {
    }

    转成ssa形式

    =================== after ssa ====================================

    v7.0 = y_72.0;
    if ( y_72.0 > 9 )
      v6.0 = 1;
    v8.0 = 1467139458;
    if ( y_72.0 > 9 )
      v8.1 = -486874641;
    v8.2 = phi(v8.0, v8.1)
    if ( v6.0 )
      v8.3 = 1467139458;

    v8.4 = phi(v8.2, v8.3)  ==> v8.4 = 1467139458 
    if ( v8 == 1467139458 ) {
    }

    v8必然等于1467139458 ，因为y_72 > 9 和 v6是等价的
    */
    int             on_cond_MULTIEQUAL();
    /*
    @return >0       top
    */
    int             on_cond_MULTIEQUAL2();
    bool            all_inrefs_is_constant(void);
    /* 所有的输入节点，都是邻接节点 */
    bool            all_inrefs_is_adj(void);
    bool            all_inrefs_is_top(void);
    /* 获取和自己第一个等于c的varnode */
    varnode*        get_const_in(varnode *c) {
        for (int i = 0; i < inrefs.size(); i++)
            if (inrefs[i]->is_constant() && (inrefs[i]->type == c->type)) return inrefs[i];
        return NULL;
    }
    /* 获取自己的第一个常量输入 */
    varnode*        get_const_in() {
        for (int i = 0; i < inrefs.size(); i++)
            if (inrefs[i]->is_constant()) return inrefs[i];
        return NULL;
    }
    varnode*        get_top_in() {
        for (int i = 0; i < inrefs.size(); i++)
            if (inrefs[i]->is_top()) return inrefs[i];
        return NULL;
    }
    void            loadram2out(Address &addr);
    void            create_stack_virtual_vn();
    /*
    这个函数主要是为了检测，在load, store路径上，当前的store的原始值是否就是来自于从这个pos的load
    比如:

    1. a = *pos1;
    2. *pos2 = a;

    假如pos1 == pos2，那返回 pcodeop 2
    */
    pcodeop*        find_same_pos_load(vector<pcodeop *> &store);
    int             compute_on_and(void);
    int             compute_on_or(void);
};

#endif
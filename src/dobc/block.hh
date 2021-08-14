
#ifndef __block_h__
#define __block_h__

#include "address.hh"
#include "pcodeop.hh"
#include <bitset>
#include "high.hh"

class varnode;
class funcdata;
class jmptable;
class dobc;
class blockgraph;

#define a_tree_edge             0x1
#define a_forward_edge          0x2
#define a_cross_edge            0x4
#define a_back_edge             0x8
#define a_loop_edge             0x10
#define a_true_edge             0x20
#define a_mark                  0x40
#define a_unpropchain           0x80
#define a_do_propchain          0x100

class blockedge {
public:
    uint32_t label;
    flowblock *point;
    int reverse_index;
    /* 存临时变量的 */
    int index;

    blockedge(flowblock *pt, int lab, int rev) { point = pt, label = lab; reverse_index = rev; }
    blockedge() {};
    bool is_true() { return label & a_true_edge;  }
    void set_true(void) { label |= a_true_edge; }
    void set_false(void) { label &= a_true_edge;  }
    void set_flag(uint32_t flag) { label |= flag;  }
    void clear_flag(uint32_t flag) { label &= ~flag;  }
    bool is(uint32_t flag) { return label & flag;  }
};


enum block_type{
    a_condition,
    a_if,
    a_whiledo,
    a_dowhile,
    a_switch,
};

class flowblock {
public:
    enum block_type     type;

    struct {
        unsigned f_goto_goto : 1;
        unsigned f_break_goto : 1;
        unsigned f_continue_goto : 1;
        unsigned f_switch_out : 1;
        unsigned f_entry_point : 1;
        /* 
        1. 在cbranch中被分析为不可达，确认为死 */
        unsigned f_dead : 1; 
        unsigned f_no_cmp : 1;

        unsigned f_switch_case : 1;
        unsigned f_switch_default : 1;

        /* 在某些算法中，做临时性标记用 

        1. reachable测试
        2. reducible测试
        3. augment dominator tree收集df
        4. djgraph 收集df
        */
        unsigned f_mark : 1;

        unsigned f_return : 1;

        /* 这个block快内有call函数 */
        unsigned f_call : 1;
        /* 不允许合并 */
        unsigned f_unsplice : 1;
        /* 内联的时候碰到的cbranch指令 */
        unsigned f_cond_cbranch : 1;
        /* 我们假设节点 e 为结束节点，节点 a -> e，当且仅当e为a的唯一出节点，那么a在e的结束路径上，
        e也在自己的路径上
        */
        unsigned f_exitpath : 1;
        /* 这个循环*/
        unsigned f_irreducible : 1;
        unsigned f_loopheader : 1;

    } flags = { 0 };

    RangeList cover;
    bitset<32>      live_gen;
    bitset<32>      live_kill;
    bitset<32>      live_in;
    bitset<32>      live_out;

    list<pcodeop*>      ops;
    list<pcodeop *>     sideeffect_ops;

    blockgraph *parent = NULL;
    flowblock *immed_dom = NULL;
    flowblock *post_immed_dom = NULL;
    /* 
    1. 标明自己属于哪个loop
    2. 假如自己哪个loop都不属于，就标空
    3. 假如一个循环内有多个节点，找dfnum最小的节点
    4. 对于一个循环内的除了loopheader的节点，这个指针指向loopheader，loopheader自己本身的
       loopheader指向的起始是外层的loop的loopheader，这个一定要记住
    */
    flowblock *loopheader = NULL;
    /* 标明这个loop有哪些节点*/
    vector<flowblock *> irreducibles;
    vector<flowblock *> loopnodes;
    /* 
    1. 测试可规约性
    2. clone web时有用
    */
    flowblock *copymap = NULL;

    /* 这个index是 后序遍历的索引，用来计算支配节点数的时候需要用到 */
    int index = 0;
    /* 这个index是 把树翻转以后，计算的 post-order index */
    int rindex = 0;
    int dfnum = 0;
    int numdesc = 1;        // 在 spaning tree中的后代数量，自己也是自己的后代，所以假如正式计算后代数量，自己起始为1

    int vm_byteindex = -1;      
    int vm_caseindex = -1;

    jmptable *jmptable = NULL;
    vector<blockedge>   in;
    vector<blockedge>   out;
    funcdata *fd;

    /* code gen */
    struct {
        /* 自己的block生成的代码对应的具体位置 */
        unsigned char *data = NULL;
    }cg;

    struct {
        /* 这个block快中，是否有call指令 */
        int             have_call;
        /* 这个block快中，是否有maystore */
        int             have_may_store;
        /* 这里的liveout和livein，都是mem相关*/
        set<varnode *>  liveout;
        set<varnode *>  livein;
    } memflow;

    high_cond   hi_cond;

    flowblock(void);
    flowblock(funcdata *fd);
    ~flowblock();

    flowblock*  get_out(int i) { return out[i].point;  }
    flowblock*  get_0out() { return (out.size() == 1) ? get_out(0) : NULL; }
    flowblock*  get_in(int i) { return in[i].point;  }
    flowblock*  get_0in() { return (in.size() == 1) ? get_in(0) : NULL;  }
    flowblock*  get_max_dfnum_in() {
        flowblock *max = get_in(0);

        for (int i = 1; i < in.size(); i++) {
            if (in[i].point->dfnum > max->dfnum) {
                max = get_in(i);
            }
        }

        return max;
    }
    flowblock*  get_min_dfnum_in() {
        flowblock *min = get_in(0);

        for (int i = 1; i < in.size(); i++) {
            if (in[i].point->dfnum < min->dfnum) {
                min = get_in(i);
            }
        }

        return min;
    }

    bool        is_in(flowblock *b) {
        return get_in_index(b) >= 0;
    }
    bool        is_out(flowblock *b) {
        return get_out_index(b) >= 0;
    }
    pcodeop*    first_op(void) { return *ops.begin();  }
    pcodeop*    last_op(void) { 
		return ops.size() ? (*--ops.end()):NULL;  
	}
    list<pcodeop *>::iterator    last_it(void) { 
        return ops.end();
	}
    int         last_order() { return last_op()->start.getOrder();  }

    int         get_out_rev_index(int i) { return out[i].reverse_index;  }
    /* 判断这个block是否指令为空

    1. phi节点不算

    @except_branch      计算指令总数时，不算branch
    */
    bool        is_empty(int except_branch);

    /* 判断这个block指令为空后，能否删除，有几种情况不删除 
    1. 有多个out边
    2. 有一条out边，但是指向自己
    */
    bool        is_empty_delete(void);

    void        set_initial_range(const Address &begin, const Address &end);
    void        add_op(pcodeop *);
    void        insert(list<pcodeop *>::iterator iter, pcodeop *inst);


    Address     get_start(void);

    bool        is_back_edge_in(int i) { return in[i].label & a_back_edge; }
    void        set_mark() { flags.f_mark = 1;  }
    void        clear_mark() { flags.f_mark = 0;  }
    bool        is_mark() { return flags.f_mark;  }
    bool        is_entry_point() { return flags.f_entry_point;  }
    bool        is_switch_out(void) { return flags.f_switch_out;  }
    int         get_in_index(const flowblock *bl);
    int         get_out_index(const flowblock *bl);

    void        add_in_edge(flowblock *b, int lab);
    int         remove_in_edge(int slot);
    void        remove_out_edge(int slot);
    void        half_delete_out_edge(int slot);
    void        half_delete_in_edge(int slot);
    int         get_back_edge_count(void);
    flowblock*  get_back_edge_node(void);
    /* 当这个block的末尾节点为cbranch节点时，返回条件为真或假的跳转地址 */
    blockedge*  get_true_edge(void);
    blockedge*  get_false_edge(void);

    flowblock*  get_true_block(void) {
        return get_true_edge()->point;
    }

    flowblock*  get_false_block(void) {
        return get_false_edge()->point;
    }
    /*
    一个带cbranch的branch，走向其中一个branch时，它的条件
    */
    int         get_branch_cond(flowblock *branch, high_cond &cond);

    void        set_out_edge_flag(int i, uint4 lab);
    void        clear_out_edge_flag(int i, uint4 lab);

    void        set_dead(void) { flags.f_dead = 1;  }
    int         is_dead(void) { return flags.f_dead;  }
    bool        is_irreducible() { return flags.f_irreducible;  }
    void        remove_op(pcodeop *inst);
    void        replace_in_edge(int num, flowblock *b);
    list<pcodeop *>::reverse_iterator get_rev_iterator(pcodeop *op);
    pcodeop*    first_callop();
    void        mark_unsplice() { flags.f_unsplice = 1;  }
    bool        is_unsplice() { return flags.f_unsplice; }
    bool        is_end() { return out.size() == 0;  }
    /* 尾部的cbranch是否是指令内的相对跳转 */
    bool        is_rel_cbranch();
    bool        is_rel_branch();
    Address     get_return_addr();
    pcodeop*    get_pcode(int pid) {
        list<pcodeop *>::iterator it;
        for (it = ops.begin(); it != ops.end(); it++) {
            if ((*it)->start.getTime() == pid)
                return *it;
        }

        return NULL;
    }
    /* 查找以这个变量为out的第一个pcode */
    pcodeop*    find_pcode_def(const Address &out);
    void        dump();
    int         sub_id();

    void        clear_loopinfo() {
        loopheader = NULL;
        loopnodes.clear();
        irreducibles.clear();
        flags.f_irreducible = 0;
        flags.f_loopheader = 0;
    }
    bool noreturn(void);
    /* 当一个block末尾是cbranch时， 一般它的比较条件是来自于cmp 
    cmp指令会产生一个sub指令，返回这个sub指令，假如没有，就返回NULL
    */
    pcodeop*    get_cbranch_sub_from_cmp(void);
    /* 寻找最后一个操作ZR寄存器的指令*/
    pcodeop*    get_last_oper_zr(void);
    /* FIXME:需要重调整个机制 */
    bool        is_iv_in_normal_loop(pcodeop *op);
    bool        is_eq_cbranch(void);
    /* 不属于任何一个循环 */
    bool        is_out_loop() {
        return !loopheader && loopnodes.empty() && !flags.f_irreducible;
    }
    bool        is_adjacent(flowblock *adj) {
        return get_in_index(adj) >= 0;
    }
    bool        is_cbranch() {
        pcodeop *p = last_op();
        return p ? (p->opcode == CPUI_CBRANCH) : false;
    }
    bool        is_it_cbranch() {
        return is_cbranch() ? last_op()->flags.itblock : 0;
    }
    /* 获取输入节点中没有被标记过的数量 */
    int         incoming_forward_branches();
    /* 当flowblock是一个分支跳转节点时，给出除了b以外的另外一个节点 */
    flowblock*  get_cbranch_xor_out(flowblock *b) {
        return (b == get_out(0)) ? get_out(1) : get_out(0);
    }
    /* 某些指令内部有循环，比如: vld2.8 {d18,d19},[r3] 
    
    拆开循环以后，内部的pcode会分成多块，这个是判断当前的cfg节点，是否是首快 */
    bool        is_rel_header() {
        return ((out.size() == 1) && get_out(0)->is_rel_cbranch());
    }

    int         get_cbranch_cond();
    /*
    计算当一个cbranch走向其中某个分支时，需要的条件
    */
    int         calc_cond(flowblock *to, high_cond &cond);
    bool        have_same_cmp_condition(flowblock *b);

    /* 
        cfgA(vn1)
        /  \ 
       vn0 /
        \ /
        phi
    
    当phi节点，是在一个if then 的交汇处产生时，尝试获取false边的vn
    */
    varnode*    get_false_vn(pcodeop *phi);
    varnode*    get_true_vn(pcodeop *phi);

    /* 对内接口
    
    -1: unknown
    0: false
    1: true
    */
    int         lead_to_edge(pcodeop *op, pcodeop *phi, varnode *vn);
    /* 当某个phi节点，被其中的一个val定值时，op节点所在的块，会走向哪一边 */
    bool        lead_to_false_edge(pcodeop *op, pcodeop *phi, varnode *val);
    bool        lead_to_true_edge(pcodeop *op, pcodeop *phi, varnode *val);

    /* 给定某个pcode，返回同一个地址的第一个pcode */
    list<pcodeop*>::iterator    find_inst_first_op(pcodeop *p);

    /*
    return  1       changed
            0       no changed
    */
    int             calc_memflow();
    /* 把自己的输入节点，按dfnum排序 */
    void           get_inlist_on_dfsort(vector<blockedge *> &inlist);
    bool            is_11_branch() {
        return ((ops.size() == 1) && (last_op()->opcode == CPUI_BRANCH) && (in.size() == 1) && (out.size() == 1));
    }
    /*
    判断一个块是否可以直达另外一个块。

    b1 -> b2 .. -> bn

    [b1, bn) 的out.size()都为1
    */
    bool            is_direct_connect_to(flowblock *to);
};

class blockgraph {
public:
    vector<flowblock *> blist;
    vector<flowblock *> deadblist;
    /* 一个函数的所有结束节点 */
    vector<flowblock *> exitlist;
    /* 有些block是不可到达的，都放到这个列表内 */
    vector<flowblock *> deadlist;

    funcdata *fd;
    dobc *d;

    /* 识别所有的循环头 */
    vector<flowblock *> loopheaders;

    flowblock exit;

    int index = 0;

    //--------------------
    blockgraph(funcdata *fd1);

    flowblock*  get_block(int i) { return blist[i]; }
    flowblock*  get_block_by_index(int index) {
        for (int i = 0; i < blist.size(); i++)
            if (blist[i]->index == index) return blist[i];

        return NULL;
    }
    int         get_size(void) { return blist.size();  }
    void        set_start_block(flowblock *bl);
    flowblock*  get_entry_point(void);
    void        clear_marks(void);

    /*
    1. 检测header是否为 while...do 形式的循环的头节点
    2. 假如不是，返回NULL
    3. 假如是，计算whiledo的结束节点是哪个
    */
    flowblock*  detect_whiledo_exit(flowblock *header);
    void        add_block(flowblock *b);
    flowblock*  new_block_basic(void);
    flowblock*  new_block_basic(intb offset);

    void        structure_loops(vector<flowblock *> &rootlist);
    void        find_spanning_tree(vector<flowblock *> &preorder, vector<flowblock *> &rootlist);
    void        dump_spanning_tree(const char *filename, vector<flowblock *> &rootlist);
    void        calc_forward_dominator(const vector<flowblock *> &rootlist);
    void        build_dom_tree(vector<vector<flowblock *> > &child);
    int         build_dom_depth(vector<int> &depth);
    /*
    寻找一种trace流的反向支配节点，

    一般的反向支配节点算法，就是普通支配节点算法的逆

    而这个算法是去掉，部分节点的回边而生成反向支配节点，用来在trace流中使用
    */
    flowblock*  find_post_tdom(flowblock *h);
    bool        find_irreducible(const vector<flowblock *> &preorder, int &irreduciblecount);

    /* loop 处理 */
    void        add_loopheader(flowblock *b) { 
        b->flags.f_loopheader = 1;
        loopheaders.push_back(b);  
        b->loopnodes.push_back(b);
    }
    bool        in_loop(flowblock *lheader, flowblock *node);
    void        clear_loopinfo() {
        loopheaders.clear();
    }

    /* 边处理 */
    void        clear(void);
    int         remove_edge(flowblock *begin, flowblock *end);
    void        add_edge(flowblock *begin, flowblock *end);
    void        add_edge(flowblock *b, flowblock *e, int label);
    void        calc_exitpath();
    void        remove_block(flowblock *bl);

    void        collect_reachable(vector<flowblock *> &res, flowblock *bl, bool un) const;
    void        splice_block(flowblock *bl);
    void        move_out_edge(flowblock *blold, int slot, flowblock *blnew);

    void        clear_all_unsplice();
    void        clear_all_vminfo();

    flowblock*  add_block_if(flowblock *b, flowblock *cond, flowblock *tc);
    bool        is_dowhile(flowblock *b);
    /* 这个函数有点问题 */
    flowblock*  find_loop_exit(flowblock *start, flowblock *end);
    /* 搜索到哪个节点为止 */
    pcodeop*    first_callop_vmp(flowblock *end);
    void        remove_from_flow(flowblock *bl);
    /* 计算每个blk 的live sets */
    void        compute_local_live_sets(void);
    void        compute_global_live_sets(void);

    /* 计算每个pcode的live set*/
    void        compute_local_live_sets_p(void);
    void        compute_global_live_sets_p(void);
    void        dump_live_sets();
    void        dump_live_set(flowblock *b);
    void        dump_live_set(pcodeop *p);
    void        collect_cond_copy_sub(vector<pcodeop *> &subs);
    void        collect_no_cmp_cbranch_block(vector<flowblock *> &blks);
    flowblock*  get_it_end_block(flowblock *b);
    void        collect_sideeffect_ops();
    flowblock*  find_post_dom(flowblock *f);
    void        calc_post_dominator();
    /*
    计算 post-dominator 时，必须得把尾部节点做为跟节点，然后计算它的post-order

    在重新应用支配节点计算算法
    */
    void        post_order_on_rtree(flowblock *root, vector<flowblock *> &postorder, vector<char> &mark);
    void        add_exit();
    void        del_exit();
};


#endif
#ifndef __funcdata_h__
#define __funcdata_h__

#include <stdio.h>
#include "sleigh.hh"
#include "pcodefunc.hh"
#include "block.hh"

class funcdata;
class varnode;
class pcodeop;
class func_call_specs;
class ollvmhead;

typedef map<Address, int> version_map;
typedef map<Address, vector<pcodeop_lite *>> instmap;

struct pcodeop_cmp_def {
    bool operator() ( const pcodeop *a, const pcodeop *b ) const;
};

typedef set<pcodeop *, pcodeop_cmp_def> pcodeop_def_set;

typedef map<Address, vector<varnode *> > variable_stack;

struct VisitStat {
    SeqNum seqnum;
    int size;
};

class op_edge {
public:
    pcodeop *from;
    pcodeop *to;
    int t = 0; // true flag

    op_edge(pcodeop *from, pcodeop *to);
    ~op_edge();
} ;

class funcproto {
public:
    struct {
        unsigned vararg : 1;        // variable argument
        unsigned exit : 1;          // 调用了这个函数会导致整个流程直接结束，比如 exit, stack_check_fail
        unsigned side_effect : 1;
    } flags = { 0 };
    /* -1 代表不知道 
    */
    int     inputs = -1;
    int     output = -1;
    string  name;
    Address addr;

    funcproto() { flags.side_effect = 1;  }
    ~funcproto() {}

    void set_side_effect(int v) { flags.side_effect = v;  }
};

class priority_queue {
public:
    vector<vector<flowblock *> > queue;
    int curdepth;

    priority_queue(void) { curdepth = -2;  }
    void reset(int maxdepth);
    void insert(flowblock *b, int depth);
    flowblock *extract();
    bool empty(void) const { return (curdepth == -1);  }
};

class rangenode {
public:
    intb    start = 0;
    int     size = 0;

    rangenode();
    ~rangenode();

    intb    end() { return start + size;  }
};

class pcodeemit2 : public PcodeEmit {
public:
    funcdata *fd = NULL;
    pcodeop *prevp = NULL;
    FILE *fp = stdout;
    int itblock = 0;
    virtual void dump(const Address &address, OpCode opc, VarnodeData *outvar, VarnodeData *vars, int size);

    void set_fp(FILE *f) { fp = f;  }
    void enter_itblock() { itblock = 1;  }
    void exit_itblock() { itblock = 0;  }
};

int print_vartype(Translate *trans, char *buf, varnode *data);

#define COLOR_ASM_INST_MNEM             "#3933ff"               
#define COLOR_ASM_INST_BODY             "#3933ff"               
#define COLOR_ASM_ADDR                  "#33A2FF"               
#define COLOR_ASM_STACK_DEPTH           "green"

class AssemblyRaw : public AssemblyEmit {

public:
    char *buf = NULL;
    FILE *fp = NULL;
    int sp = 0;
    int enable_html = 1;
    int mnem1 = 0;

    virtual void dump(const Address &addr, const string &mnem, const string &body);


    void set_mnem(int m) { mnem1 = m; }
    void set_buf(char *b) { buf = b; }
    void set_fp(FILE *f) { fp = f; }
    void set_sp(int s) { sp = s;  };
    void disable_html() { enable_html = 0;  }
};

struct pcodeop_cmp {
    bool operator() ( const pcodeop *a, const pcodeop *b ) const;
};

typedef map<SeqNum, pcodeop *>  pcodeop_tree;

typedef map<pcodeop *, valuetype, pcodeop_cmp> valuemap;

typedef set<pcodeop *, pcodeop_cmp> pcodeop_set;


class jmptable {
public:
    pcodeop *op;
    Address opaddr;
    int defaultblock;
    int lastblock;
    int size;

    vector<Address>     addresstable;

    jmptable(pcodeop *op);
    jmptable(const jmptable *op2);
    ~jmptable();

    void    update(funcdata *fd);
};


class funcdata {
public:
    struct {
        unsigned blocks_generated : 1;
        unsigned blocks_unreachable : 1;    // 有block无法到达
        unsigned processing_started : 1;
        unsigned processing_complete : 1;
        unsigned no_code : 1;
        unsigned unimplemented_present : 1;
        unsigned baddata_present : 1;

        unsigned safezone : 1;
        unsigned plt : 1;               // 是否是外部导入符号
		/* 是否允许标记未识别store，让安全store可以跨过去这个pcode*/
		unsigned enable_topstore_mark : 1;
		/* liverange有2种计算类型
		
		1. 一种是快速但不完全，可以做peephole，不能做register allocation
		2. 一种是慢速但完全，可以参与所有优化 */
		unsigned enable_complete_liverange : 1;
        unsigned thumb : 1;
        unsigned dump_inst : 1;
        /* 关闭常量持久化，比如
        copy r0, 1
        cmp r1, r0


        ==> 

        copy r0, 1
        cmp r1, 1
        
        关闭掉这种优化
        */
        unsigned disable_to_const : 1;
        unsigned disable_inrefs_to_const : 1;
        /* 关闭simid指令的常量化，否则处理起来很麻烦 */
        unsigned disable_simd_to_const : 1;

        unsigned noreturn : 1;
    } flags = { 0 };

    enum {
        a_local,
        a_global,
        a_plt,
    } symtype;

    int op_generated = 0;
	int reset_version = 0;
    int heritage_times = 0;

    pcodeop_tree     optree;
    funcproto       funcp;

    /*
    静态trace的时候，记录原始的数据值，在执行完毕后，做恢复用

    每次使用之前，必须得执行clear
    */
    valuemap    tracemap;

    /* 用来还原指令用的，我们在做代码生成的时候，可以比对原先的指令差距，假如和原先的指令没有差别，
    可以直接采用原先的指令  */
    instmap     litemap;

    /* 记录地址和版本号的,SSA时用到 */
    version_map vermap;

    /* jmp table */
    vector<pcodeop *>   tablelist;
    vector<jmptable *>  jmpvec;

    /* op_gen_iter 用来分析ops时用到的，它指向上一次分析到的pcode终点 */
    list<pcodeop *>::iterator op_gen_iter;
    /* deadlist用来存放所有pcode */
    list<pcodeop *>     deadlist;
    list<pcodeop *>     alivelist;
    list<pcodeop *>     storelist;
    list<pcodeop *>     loadlist;
    list<pcodeop *>     useroplist;
    list<pcodeop *>     deadandgone;
    list<pcodeop *>     calllist;
    list<pcodeop *>     philist;

    vector<blockedge *>   propchains;

    /* 函数出口活跃的变量 */
    pcodeop_def_set topname;
    intb user_step = 0x10000;
    intb user_offset = 0x10000;
    int op_uniqid = 0;

    map<Address,VisitStat> visited;
    dobc *d = NULL;

    flowblock * vmhead = NULL;
    Address *iv = NULL;

    /* vbank------------------------- */
    struct {
        long uniqbase = 0;
        int uniqid = 0;
        int create_index = 0;
    } vbank;

    varnode_loc_set     loc_tree;
    varnode_def_set     def_tree;
    varnode             searchvn;
    /* vbank------------------------- */

    /* control-flow graph */
    blockgraph bblocks;

    list<op_edge *>       block_edge;

    int     intput;         // 这个函数有几个输入参数
    int     output;         // 有几个输出参数
    list<func_call_specs *>     qlst;

    /* heritage start ................. */
    vector<vector<flowblock *> > domchild;
    vector<vector<flowblock *> > augment;
#define boundary_node       1
#define mark_node           2
#define merged_node         4
#define visit_node          8
    vector<uint4>   phiflags;   
    vector<int>     domdepth;
    /* dominate frontier */
    vector<flowblock *>     merge;      // 哪些block包含phi节点
    priority_queue pq;

    int maxdepth = -1;

    LocationMap     disjoint;
    LocationMap     globaldisjoint;

    /* 这个以前是Ghidra里面用来多次做heritage的，因为普通的ssa和mem-ssa必须要分开做 */
    int pass = 0;

    /* heritage end  ============================================= */
    vector<pcodeop *>   trace;
    int             virtualbase = 0x10000;
    /*---*/

    Address startaddr;

    Address baddr;
    Address eaddr;
    string name;
    string alias;

    /* 扫描到的最小和最大指令地址 */
    Address minaddr;
    Address maxaddr;
    int inst_count = 0;
    int inst_max = 1000000;

    /* 这个区域内的所有可以安全做别名分析的点 */

    /* vmp360--------- */
    list<rangenode *> safezone;
    intb     vmeip = 0;
    /* vmp360  end--------- */

    /* ollvm */
    struct {
        vector<ollvmhead *>   heads;
    } ollvm;

    pcodefunc pf;

    vector<Address>     addrlist;
    /* 常量cbranch列表 */
    vector<pcodeop *>       cbrlist;
    vector<flowblock *>     emptylist;
    pcodeemit2 emitter;

    /* 做条件inline时用到 */
    funcdata *caller = NULL;
    pcodeop *callop = NULL;

    /* elf的symbol指示的大小 */
    int symsize;

    struct {
        int     size;
        uint8_t *bottom;
        uint8_t *top;
    } memstack;

    funcdata(const char *name, const Address &a, int symsiz, dobc *d);
    ~funcdata(void);

    const Address&  get_addr(void) { return startaddr;  }
    string&     get_name() { return name;  }
    void        set_alias(string a) { alias = a;  }
    string&     get_alias(void) { return alias;  }
    void        set_range(Address &b, Address &e) { baddr = b; eaddr = e; }
    void        set_op_uniqid(int val) { op_uniqid = val;  }
    int         get_op_uniqid() { return op_uniqid; }
    void        set_user_offset(int v) { user_offset = v;  }
    int         get_user_offset() { return user_offset; }
    void        set_virtualbase(int v) { virtualbase = v;  }
    int         get_virtualbase(void) { return virtualbase; }
    void        set_thumb(int thumb) { flags.thumb = thumb;  }
    int         get_thumb(void) { return flags.thumb;  }

    pcodeop*    newop(int inputs, const SeqNum &sq);
    pcodeop*    newop(int inputs, const Address &pc);
    pcodeop*    cloneop(pcodeop *op, const SeqNum &seq);
    /* 
    克隆一个类似的结构，但是不插入Optree，deadlist, 也不需要seq，这个只是为了做结构比较用的，

    只保留最基本的信息，包括opcode, output, inrefs, address
    */
    pcodeop_lite*   cloneop_lite(pcodeop *op);
    void        op_destroy_raw(pcodeop *op);
    void        op_destroy(pcodeop *op);
    void        op_destroy_ssa(pcodeop *op);
	void		op_destroy(pcodeop *op, int remove);
    void        total_replace(varnode *vn, varnode *newvn);
	void		remove_all_dead_op();
    void        reset_out_use(pcodeop *p);

    varnode*    new_varnode_out(int s, const Address &m, pcodeop *op);
    varnode*    new_varnode(int s, AddrSpace *base, uintb off);
    varnode*    new_varnode(int s, const Address &m);
    /* new_coderef是用来创建一些程序位置的引用点的，但是这个函数严格来说是错的····，因为标识函数在pcode
    的体系中，多个位置他们的address可能是一样的
    */
    varnode*    new_coderef(const Address &m);
    varnode*    new_unique(int s);
    varnode*    new_unique_out(int s, pcodeop *op);
    varnode*    new_mem(AddrSpace *spc, int offset, int s);

    varnode*    clone_varnode(const varnode *vn);
    void        destroy_varnode(varnode *vn);
    void        delete_varnode(varnode *vn);
    /* 设置输入参数 */
    varnode*    set_input_varnode(varnode *vn);

    varnode*    create_vn(int s, const Address &m);
    varnode*    create_def(int s, const Address &m, pcodeop *op);
    varnode*    create_def_unique(int s, pcodeop *op);
    varnode*    create_constant_vn(intb val, int size);
    varnode*    xref(varnode *vn);
    varnode*    set_def(varnode *vn, pcodeop *op);

    void        op_resize(pcodeop *op, int size);
    void        op_set_opcode(pcodeop *op, OpCode opc);
    void        op_set_input(pcodeop *op, varnode *vn, int slot);
    void        op_set_output(pcodeop *op, varnode *vn);
    void        op_unset_input(pcodeop *op, int slot);
    void        op_unset_output(pcodeop *op);
    void        op_remove_input(pcodeop *op, int slot);
    void        op_insert_input(pcodeop *op, varnode *vn, int slot);
    void        op_zero_multi(pcodeop *op);
    void        op_unlink(pcodeop *op);
    void        op_uninsert(pcodeop *op);
    void        clear_block_phi(flowblock *b);
    void        clear_block_df_phi(flowblock *b);

    pcodeop*    find_op(const Address &addr);
    pcodeop*    find_op(const SeqNum &num) const;
    void        del_op(pcodeop *op);
    void        del_varnode(varnode *vn);

    varnode_loc_set::const_iterator     begin_loc(const Address &addr);
    varnode_loc_set::const_iterator     end_loc(const Address &addr);
    varnode_loc_set::const_iterator     begin_loc(AddrSpace *spaceid);
    varnode_loc_set::const_iterator     end_loc(AddrSpace *spaceid);

    void        del_remaining_ops(list<pcodeop *>::const_iterator oiter);
    void        new_address(pcodeop *from, const Address &to);
    pcodeop*    find_rel_target(pcodeop *op, Address &res) const;
    pcodeop*    target(const Address &addr) const;
    pcodeop*    branch_target(pcodeop *op);
    pcodeop*    fallthru_op(pcodeop *op);

    bool        set_fallthru_bound(Address &bound);
    void        fallthru();
    pcodeop*    xref_control_flow(list<pcodeop *>::const_iterator oiter, bool &startbasic, bool &isfallthru);
    void        generate_ops_start(void);
    void        generate_ops(void);
    bool        process_instruction(const Address &curaddr, bool &startbasic);
    void        recover_jmptable(pcodeop *op, int indexsize);
    void        analysis_jmptable(pcodeop *op);
    jmptable*   find_jmptable(pcodeop *op);
    int         get_size() { return (int)(maxaddr.getOffset() - minaddr.getOffset());  }

    void        collect_edges();
    void        generate_blocks();
    void        split_basic();
    void        connect_basic();

    void        dump_inst();
    void        dump_block(FILE *fp, flowblock *b, int pcode);
    /* flag: 1: enable pcode */
    void        dump_cfg(const string &name, const char *postfix, int flag);
    void        dump_pcode(const char *postfix);
    void        dump_rank(FILE *fp);
    /* 打印loop的包含关系 */
    void        dump_loop(const char *postfix);
    /* dump dom-joint graph */
    void        dump_djgraph(const char *postfix, int flag);
	void        dump_liverange(const char *postfix);

    void        op_insert_before(pcodeop *op, pcodeop *follow);
    void        op_insert_after(pcodeop *op, pcodeop *prev);
    void        op_insert(pcodeop *op, flowblock *bl, list<pcodeop *>::iterator iter);
    void        op_insert_begin(pcodeop *op, flowblock *bl);
    void        op_insert_end(pcodeop *op, flowblock *bl);
    void        inline_flow(funcdata *inlinefd, pcodeop *fd);
    void        inline_clone(funcdata *inelinefd, const Address &retaddr);
    void        inline_call(string name, int num);
    void        inline_call(const Address &addr, int num);
    /* 条件inline
    
    当我们需要inline一个函数的时候，某些时候可能不需要inline他的全部代码，只inline部分

    比如 一个vmp_ops函数

    function vmp_ops(VMState *s, int val1, int val2) {
        optype = stack_top(s);
        if (optype == 17) {
            vmp_ops2(s, val1, val2);
        }
        else if (optype == 16) {
        }
        else {
        }
    }

    假如我们外层代码在调用vmp_ops的时候，代码如下

    stack_push(s, 17);
    vmp_ops(s, 1, 2);

    那么实际上vmp_ops进入以后，只会进入17的那个分支，我们在inline一个函数时，
    把当前的上下文环境传入进去(记住这里不是模拟执行)，然后在vmp_ops内做编译优化，
    并对它调用的函数，也做同样的cond_inline。

    这种奇怪的优化是用来对抗vmp保护的，一般的vmp他们在解构函数时，会形成大量的opcode
    然后这些opcode，理论上是可以放到一个大的switch里面处理掉的，有些写壳的作者会硬是把
    这个大的switch表拆成多个函数
    */
    flowblock*  argument_inline(funcdata *inlinefd, pcodeop *fd);
    void        argument_pass(void);
    void        set_caller(funcdata *caller, pcodeop *callop);

    void        inline_ezclone(funcdata *fd, const Address &calladdr);
    bool        check_ezmodel(void);
    void        structure_reset();

    void        mark_dead(pcodeop *op);
    void        mark_alive(pcodeop *op);
    void        mark_free(varnode *vn);
    void        fix_jmptable();
    char*       block_color(flowblock *b);
    char*       edge_color(blockedge *e);
    int         edge_width(blockedge *e);
    void        start_processing(void);
    void        follow_flow(void);
    void        add_callspec(pcodeop *p, funcdata *fd);
    void        clear_blocks();
    void        clear_blocks_mark();
    int         inst_size(const Address &addr);
    void        build_adt(void);
    void        calc_phi_placement(const vector<varnode *> &write);
    void        calc_phi_placement2(const vector<varnode *> &write);
    void        calc_phi_placement3(const vector<flowblock *> &write);

    /*
    计算特殊情况下unique space上的变量的phi插入位置，可以直接在本文件内搜outlive
    */
    void        calc_phi_placement4(const vector<varnode *> &write);
    void        visit_dj(flowblock *cur,  flowblock *v);
    void        visit_incr(flowblock *qnode, flowblock *vnode);
    void        place_multiequal(void);
    void        rename();
    /*
    @vermap 用来标注版本号的一个map，一般的SSA算法里面携带的版本号，其实最大的意义是用来标识2个同名var是否一样，本身版本号
            的大小是无意义的，Ghidra意识到了这一点，所以它的varnode本身不带版本号，所有的同版本varnode，就是同一个vn的指针。
            但是这个不方便我们调试，所以我加了一个vermap，硬给每个vn加了一个版本。
    */
    void        rename_recurse(flowblock *bl, variable_stack &varstack, version_map &vermap);
    /* 建立活跃链，这个活跃链非常耗时，有个 build_liverange_simple，比这个快100倍以上，假如你知道simple和这个的区别，就根据自己需要
       来使用不同的版本，假如不知道就用这个，这个就是标准的活跃链建立算法 */
	void		build_liverange();
    void        build_liverange_recurse(flowblock *bl, variable_stack &varstack);

    /* 
    这个是因为某些指令内部自带相对跳转，需要指示当前的write的变量所在的block是否在这个相对跳转内
    */
    int         collect(Address addr, int size, vector<varnode *> &read,
        vector<varnode *> &write, vector<varnode *> &input, int &flags);
    void        heritage(void);
    void        heritage_clear(void);
    bool        refinement(const Address &addr, int size, const vector<varnode *> &readvars, const vector<varnode *> &writevars, const vector<varnode *> &inputvars);
    void        build_refinement(vector<int> &refine, const Address &addr, int size, const vector<varnode *> &vnlist);
    void        remove13refinement(vector<int> &refine);
    void        refine_read(varnode *vn, const Address &addr, const vector<int> &refine);
    void        refine_write(varnode *vn, const Address &addr, const vector<int> &refine);
    void        refine_input(varnode *vn, const Address &addr, const vector<int> &refine);
    varnode*    concat_pieces(const vector<varnode *> &vnlist, pcodeop *insertop, varnode *finalvn);
    /*
    \param vn 需要切割的varnode
    \param addr refine数组的起始地址
    \param refine 
    \param split 切割以后的节点
    */
    void        split_by_refinement(varnode *vn, const Address &addr, const vector<int> &refine, vector<varnode *> &split);
    /*

    */
    void        split_pieces(const vector<varnode *> &vnlist, pcodeop *insertop, const Address &addr, int size, varnode *startvn);
    /* 重构新的常量传播算法，尝试解决版本3遗留的几个问题

    1. 别名搜索效率过低，它针对每个load都会
    1.1 搜索自己的所有前驱
    1.2 假如自己的前驱有多个节点，找到自己的支配节点，搜索自己的到支配节点的所有路径
        (1) 假如中途碰到任意一个call节点
        (2) 假如中途碰到任意一个top store节点
        (3) 碰到任意一个store节点，valuetype和自己相等
        立即停止
    1.3 一直到头为止
    2. 没有支持mem-phi

    同时标准的mem-phi最大的问题，插入了太多的may def, may use节点，维护ssa的开销其实也挺大

    我们在新版本中，尝试解决部分问题

    1. mem-phi:不插入may def和may use
    2. mem-phi:只对非常少部分的mem节点，插入phi，往往是基于某些特殊需求
    3. 常量传播版本3: 

    术语:因为我把格的术语和编译器的混在了一起，所以 top store和 may store 是等价的。 

    具体:
    1. 关于效率的问题，我们新的改造如下
    1.1 正常进行常量传播，但是先关闭以前的store_query
    1.2 常量传播结束
        (1) 开始 mem rename(我们没有普通的ssa的 phi-place的过程，之所以没有，是因为我个人觉的这个时候你做的phi-place的插入很可能是错的，因为你不清楚算不清的top store，是不是真的算不清)
        (2) 维护一个 stack_addr_map<stack_addr, varnode>，它的key是stack地址，后面会考虑支持非stack地址，但是现在不管。
        (3) 扫描一个块
            [1]. 碰到may store，对整个stack_addr_map加一个may store层
            [2]. 碰到了must store，直接加入到 stack_addr_map[store] = pcode
            [3]. 碰到一个load，检查是否 top load，是的话跳过。不是的话，假如碰到了may store层，直接跳过，假如没有，查找stack_addr_map[load.addr],找到的话，进行rename.
            [4]. 记录所有livein 的must load
            [5]. 记录所有liveout 的must store
        (4) 递归扫描整个dom树，和普通的rename过程一致
    */
    int         constant_propagation4();
    int         pure_constant_propagation(pcodeop_set &set);
    /*
    1. 根据第一次heritage的结果，对所有有地址的load, store，生成sp-mem 节点

    2. 生成variable_stack_map，记录当前这个深度的所有活地址，然后开始rename

    3. 假如block的输入边是back-edge，加入mask-layer，所有的地址无效

    4. 进入block内，假如是sp-load，扫描是否当前有活的sp-store，有的话，rename
    4.1 这样关联以后，sp-load的out指向的变量值可能会变为 !top，假如是 !top ，假如到扫描列表，送入 pure_constant_propagation

    5. 假如是sp-store，送入 variable_stack_map

    6. 碰到函数，加入 mask-layer，所有地址无效（假如你精确的算出了函数的参数则例外，比如函数add，就送了r0, r1进去）

    7. 碰到may store，加入mask-layer，所有地址无效

    8. 走到block出口，统计 {bool_may_store, bool_call, liveout_store_list}
    */
    int         mem_rename();
    void        mem_rename_recurse(flowblock *bl, variable_stack &varstack, version_map &vermap);

    int         cond_constant_propagation();
    int         in_cbrlist(pcodeop *op) {
        for (int i = 0; i < cbrlist.size(); i++) {
            if (cbrlist[i] == op)
                return 1;
        }

        return 0;
    }
    /* compute sp要计算必须得满足2个要求
    
    1. block 已经被generated
    2. constant_propagation 被执行过
    */
    void        compute_sp(void);
    bool        is_code(varnode *v, varnode *v1);
    bool        is_sp_constant(varnode *v);

    void        set_safezone(intb addr, int size);
    bool        in_safezone(intb addr, int size);
    void        enable_safezone(void);
    void        disable_safezone(void);

    intb        get_stack_value(intb offset, int size);
    void        set_stack_value(intb offset, int size, intb val);
    void        add_to_codelist(pcodeop *op);
    void        remove_from_codelist(pcodeop *op);

    void        set_plt(int v) { flags.plt = v; };
    bool        test_hard_inline_restrictions(funcdata *inlinefd, pcodeop *op, Address &retaddr);
    bool        is_first_op(pcodeop *op);

    /* 获取loop 的头节点的in 节点，假如有多个，按index顺序取一个 */
    flowblock*  loop_pre_get(flowblock *h, int index);
    bool        trace_push(pcodeop *op);
    void        trace_push_op(pcodeop *op);
    void        trace_clear();
    pcodeop*    trace_store_query(pcodeop *load);
    /* 查询某个load是来自于哪个store，有2种查询方式，
    
    一种是直接指明load，后面的b可以填空，从这个load开始往上搜索
    一种是不指明load，但是指明b，从这个block开始搜索

    @load       要从这条load开始搜索，pos也来自于这个load
    @b          要从这个block开始搜搜，pos来自于外部提供，b的优先级低于load
    @pos        要搜索位置
    @maystore   当发现无法判断的store，返回这个store
    */
    pcodeop*    store_query(pcodeop *load, flowblock *b, varnode *pos, pcodeop **maystore);

#define _DUMP_PCODE             0x01
#define _DUMP_ORIG_CASE         0x02
    /* 循环展开的假如是 while switch case 里的分支则需要clone，假如不是的话则不需要复制后面的流 */
#define _DONT_CLONE             0x08
#define _NOTE_VMBYTEINDEX       0x10 
    bool        loop_unrolling4(flowblock *h, int times, uint32_t flags);

    /* 搜索从某个节点开始到某个节点的，所有in节点的集合 */
    int         collect_blocks_to_node(vector<flowblock *> &blks, flowblock *start, flowblock *end);
    /*

    @h          起始节点
    @enter      循环展开的头位置
    @end        循环展开的结束位置，不包含end，
                当循环粘展开到最后一个节点，跳出循环时，终止节点就变成了exit节点
    */
    flowblock*  loop_unrolling(flowblock *h, flowblock *end, uint32_t flags, int &meet_exit);
    int         loop_dfa_connect(uint32_t flags);
    /* 这里的dce加了一个数组参数，用来表示只有当删除的pcode在这个数组里才允许删除 这个是为了方便调试以及还原 */
#define RDS_0               1
#define RDS_UNROLL0         2
#define F_REMOVE_DEAD_PHI   4

    void        dead_code_elimination(vector<flowblock *> &blks, uint32_t flags);
    void        dead_phi_elimination();
    /*
    return  0           phi not dead
            1           phi dead
    */
    int         dead_phi_detect(pcodeop *phi, vector<pcodeop *> &deadlist);
    flowblock*  get_vmhead(void);
    flowblock*  get_vmhead_unroll(void);
    pcodeop*    get_vmcall(flowblock *b);
    Address&    get_vmhead_iv();

    flowblock*  ollvm_get_head(void);
    int         ollvm_detect_frameworkinfo();
#define b_set_flag(f, fl)             f |= fl
#define b_clear_flag(f, fl)           f &= ~fl
#define b_is_flag(f, fl)              f & fl

#define F_OPEN_COPY             0x01
#define F_OPEN_PHI              0x02
    int         ollvm_detect_propchain3(flowblock *&from, blockedge *&outedge);

    /*

    @return     0           success
                -1          no cmp node
                -2          need heritage again
    */
#define ERR_OK                      0
#define ERR_NOT_DETECT_PROPCHAIN    -1
#define ERR_NEED_REDETECT           -2
#define ERR_NO_CMP_PCODE            -3
    int         ollvm_detect_propchain4(ollvmhead *oh, flowblock *&from, blockedge *&outedge, uint32_t flags);

    void        ollvm_collect_safezone(pcodeop *phi, pcodeop_set &visit, pcodeop_set &pos_set, int depth);

    /*
    找寻某个p节点的，第一个const_def，当有多个in节点时，按dfnum排序

    @return     true    找到
                false   未找到
    */
    bool        ollvm_find_first_const_def(pcodeop *p, int outslot, flowblock *&from, blockedge *&edge, pcodeop_set &visit);

    int         ollvm_on_unconst_def(pcodeop *p1, flowblock *pre, flowblock *cur);

    int         ollvm_detect_propchains2(flowblock *&from, blockedge *&outedge);
    int         ollvm_detect_fsm2(ollvmhead *oh);
    int         ollvm_check_fsm(pcodeop *op);

    bool        use_outside(varnode *vn);
    void        use2undef(varnode *vn);
    void        branch_remove(flowblock *bb, int num);
	/* 把bb和第num个节点的关系去除 */
    void        branch_remove_internal(flowblock *bb, int num);
    void        block_remove_internal(flowblock *bb, bool unreachable);
    bool        remove_unreachable_blocks(bool issuewarnning, bool checkexistence);
    void        splice_block_basic(flowblock *bl);
    void        remove_empty_block(flowblock *bl);

    void        redundbranch_apply();
    void        dump_store_info(const char *postfix);
    void        dump_load_info(const char *postfix);
    void        dump_alias_info(FILE *fp);

    /* 循环展开时用，从start节点开始，搜索start可以到的所有节点到 end为止，全部复制出来
    最后的web包含start，不包含end */
    flowblock*  clone_web(flowblock *start, flowblock *end, vector<flowblock *> &cloneblks);
    flowblock*  clone_ifweb(flowblock *newstart, flowblock *start, flowblock *end, vector<flowblock *> &cloneblks);
	flowblock*	inline_call(pcodeop *callop, funcdata *fd);
#define F_OMIT_RETURN       1
    flowblock*  clone_block(flowblock *f, unsigned flags);
    /* 把某个block从某个位置开始切割成2块，

    比如

    1. mov r0, 1
    2. call xxx
    3. mov r1, r0
    我们内联inline了xxx以后，xxx可能是 if .. else 的结构
    那么我们需要把整个快拆开
    */
    flowblock*  split_block(flowblock *f, list<pcodeop *>::iterator it);

    char*       get_dir(char *buf);

    bool        have_side_effect(void) { return funcp.flags.side_effect;  }
    bool        have_side_effect(pcodeop *op, varnode *pos);
    void        alias_clear(void);
    /* 循环展开时用
    
    do {
        inst1
    } while (cond)

    转成

    inst1
    if (cond) {
        do {
            inst1
        } while (cond)
    }

    然后inst1在解码完以后，会得出cond的条件，假如为真，则继续展开

    inst1
    inst1
    if (cond) {
        do {
            inst1
        } while (cond)
    }
    一直到cond条件为假，删除整个if块即可
    */
    flowblock*  dowhile2ifwhile(vector<flowblock *> &dowhile);
    char*       print_indent();
    /*
    判断是否是如下类型的结构

        top
        /  \
      middle \
        \   /
        bottom 

    这个函数和判断是否cbranch是不一样的，那个只需要判断cfg节点是否已cbranch结尾，这个需要
    判断cfg节点出口的2个节点，能否立即在某个点汇合。
    */
    bool        is_ifthenfi_structure(flowblock *top, flowblock *middle, flowblock *bottom);
    /*
    v1.0 = c1
    if (xx)
        v1.1 = c2
    v1.2 = phi(v1.0, v1.1)

    判断某个phi节点，是否是上文中的结构
    */
    bool        is_ifthenfi_phi(pcodeop *phi, varnode *&top, varnode *&branch);
    /* 更严格的别名测试 */
    bool        test_strict_alias(pcodeop *load, pcodeop *store);
    /* 删除死去的store

    以前的代码，一次只能删除一个死去的store

    新的版本，会递归删除
    */
    void        remove_dead_store(flowblock *b);
    void        remove_dead_stores();
    /* 打印某个节点的插入为止*/
    void        dump_phi_placement(int bid, int pid);
    bool        is_out_live(pcodeop *op);
    /* 搜索归纳变量 */
    varnode*    detect_induct_variable(flowblock *h, flowblock *&exit);
    bool        can_analysis(flowblock *b);

    /* 
    
    把一个 
    a->h
    b->h的结构转换成
    a->c->h
    b->c->h
    @return     c
    */
    flowblock*  combine_multi_in_before_loop(vector<flowblock *> ins, flowblock *header);
    void        dump_exe();
    /* 检测可计算循环 */
    void        detect_calced_loops(vector<flowblock *> &loops);
/* 找到某个循环出口活跃的变量集合 */
    void        remove_loop_livein_varnode(flowblock *lheader);
    /* action:删除可计算循环
    lheader: 
    */
    void        remove_calculated_loop(flowblock *lheader);
    /* action: 删除所有可计算循环，并*/
    void        remove_calculated_loops();
    /*
    条件拷贝传递优化，情况1
    action:
    首先: a = T;

    figure.A
    001: b.0 = 1
    002: if (a.0 match const_cond) b.1 = 2;
    003: b.3 = phi(b.0, b.1)
    ...: ...
    ...: ...
    n+0: ...
    n+1: copy c.0, b.3

    条件传递复写成:
    figure.B
    001: b.0 = a.0
    ...
    ...
    n+0: if (b.0 match const_cond) b.1 = 2
    n+1: else b.2 = 3;
    n+2: b.3 = phi(b.1, b.2)
    n+2: copy c.0, b.3

    这里要注意的是，条件cp块的封闭性，

    什么是封闭性: 自己发明的一个概念 :)， 从一个问题开始吧？
    
    你要把一行代码从一个地方A. cut & paste 到另外一个地方B，要满足什么条件?

    1. 这行代码的out节点，必须得在地方B处活跃
    2. 这行代码的in节点，也必须得在地方B处活跃，假如不活跃，必须得通过某种方式保存下来，比如堆栈。

    你假如要把一整块代码从一个地方A. cut & paste 到另外一个地方B，要满足什么条件？

    1. 因为块代码的复杂性，我们只讲述其中一种情况
    2. 块A所有out节点，得在 B 处活跃。假如没有在B处活跃，必须得死在块A内。
    3. 块A所有in节点，最好都死在块A内，否则在B处活跃。
    4. input节点要B处活跃，否则要自己想办法保留下来，然后在地方B处恢复
    */
    int         cond_copy_propagation(varnode *phi);
    /*
    action:
    假设有一条拷贝指令，他的rn操作数，是多个常量构成，但是赋值点都太远，我们对其进行重新，方便做de-ollvm

    rd = rn

    -->

    if (rn == cl) {
        rd = c1;
    }
    else if (rn == c2) {
        rd = c2;
    }
    else 
        rd = rn;

    这个改完以后，程序的语义没有发生任何变化
    */
    int         cond_copy_expand(pcodeop *p, flowblock *b, int outslot);

    /* 测试是否需要 copy_expand，必须是Phi节点 */
    int         ollvm_copy_expand_vmhead_phi(ollvmhead *oh);
    int         ollvm_copy_expand_all_vmhead();
    int         ollvm_do_copy_expand(pcodeop *p, flowblock *b, int outslot);

    /*
    收集vn的所有def，维护一个队列

    1. v来自于copy，扫描in
    2. v来自于phi，扫描phi的所有in
    3. 不是copy，也不是phi，认为其实一个定值

    @defs       这个数组里存了扫描到的常量，已去重
    @dfnum      这个值表示，里面有多少次常量的定义，不去重
    @return     0           当前v的所有def都是常量，defs里的def就是v的所有def
                1           v有值类型为top的def
    */
    int         collect_all_const_defs(pcodeop *start, vector<varnode *> &defs, int &dfnum);
    /* 裁剪由collect_all_const_defs收集到的常量定义 

    有如下代码:

    1:  cpy r2, r0
    2:  cmp r0, #ebfa4275
    3.  beq r370a

    我们收集到了r2的所有常量定义，都不等于 #bfa4275，那么进行常量条件展开是无意义的，
    我们会从defs里面删除不等于对应常量的常量
    */
    int         cut_const_defs_on_condition(pcodeop *start, vector<varnode *> &defs);
    /*
    label0: 
        cmp r0, r1
        goto label4; 
    label1: 
        cmp r1, r2
        goto label4; 

    label:
        bgt label5

    全部修改成:

    */
    void        rewrite_no_sub_cbranch_blk(flowblock *b);
    void        rewrite_no_sub_cbranch_blks(vector<flowblock *> &blks);

    /**/

    /* 合并这些block中，最长公共尾串 
    action:

    比如:

    a -> c
    b -> c

    a:
    op1
    op2
    op7
    op8
    op10

    b:
    op3
    op5
    op7
    op8
    op10

    那么op7, 8, 10可以重新生成一个新的Block d，最后块的关系

    a -> d -> c
    b -> d -> c

    @return         0           没有合并
                    1           合并成功
    */
    int         combine_lcts(vector<flowblock *> &blks);
    /* 尝试去搜索一个block的所有in节点，找到 */
    int         combine_lcts_blk(flowblock *b);
    int         combine_lcts_all(void);

    int         ollvm_combine_lcts(pcodeop *p);

    int         cmp_itblock_cbranch_conditions(pcodeop *cbr1, pcodeop* cbr2);

    /* 针对不同的加壳程序生成不同的vmeip检测代码 */
    bool        vmp360_detect_vmeip();
    /* FIXME:应该算是代码中最重的硬编码，有在尝试去理解整个VMP框架的堆栈部分，

    360的vmp堆栈入口部分分为以下几部分:

    1. call convection prilogue save
       stmdb sp!,{r4 r5 r6 lr}
       入口部分，保护寄存器
    2. local variable 
       sub sp,sp,#0x30
       上面的0x30可能是任意值，应该是原始函数分配的堆栈大小，后面的0x100，应该是框架额外的扩展
    3. vmp framework extend
       sub sp,sp,#0x100
       vmp框架扩展的
    4. save sp 
    5. save all regs except sp
    6. save cpsr
    7. save cpsr
    8. call convections prilogue save
       7, 8都处理了2次，不是很懂360想做什么
    9. 开辟了vmp的 stack
       sub sp,sp,#0x34

    结束，大概vmp进入函数的堆栈基本就是这样
    */
    int         vmp360_detect_safezone();
    int         vmp360_detect_framework_info();
    int         vmp360_deshell();
    /* 标注程序中的堆栈中，某些360的重要字段，方便分析，这个只是在前期的debug有用，
    实际优化中是用不到的，所以它不属于硬编码 */
    void        vmp360_marker(pcodeop *op);
    pcodeop*    lastop() { return deadlist.back(); }

    int         ollvm_deshell();

    /*
    静态执行trace
    */
    int         static_trace(pcodeop *op, int inslot, flowblock **branch);
    /*
    还原静态执行trace，产生的output值变化
    */
    void        static_trace_restore();
    bool        is_safe_sp_vn(varnode *vn);
    void        set_noreturn(int v) { flags.noreturn = v;  }
    bool        noreturn() { return flags.noreturn; }

    /* 在代码生成时，是否使用以前的指令进行填充，需要做一些模式的匹配 */
    bool        use_old_inst(vector<pcodeop *> &plist);
};

#endif

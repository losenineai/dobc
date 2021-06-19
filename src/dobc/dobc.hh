

#ifndef __dobc_h__
#define __dobc_h__

#include "mcore/mcore.h"
#include "elfloadimage.hh"
#include "types.h"
#include "heritage.hh"
#include "pcodefunc.hh"
#include "funcdata.hh"
#include <bitset>

typedef struct flowblock    flowblock, blockbasic;
typedef struct jmptable     jmptable;
typedef struct funcproto    funcproto;
typedef struct rangenode    rangenode;
typedef struct func_call_specs  func_call_specs;
typedef map<Address, vector<varnode *> > variable_stack;
typedef map<Address, int> version_map;
typedef struct cover		cover;
typedef struct coverblock	coverblock;
typedef struct ollvmhead    ollvmhead;

class pcodeop;
class dobc;
class valuetype;

#define pi0(p)              p->get_in(0)
#define pi1(p)              p->get_in(1)
#define pi1(p)              p->get_in(1)
#define pi2(p)              p->get_in(2)
#define pi0a(p)             p->get_in(0)->get_addr()
#define pi1a(p)             p->get_in(1)->get_addr()
#define pi2a(p)             p->get_in(2)->get_addr()
#define poa(p)              p->output->get_addr()

#define BIT0(b)             ((b) & 0x1)
#define BIT1(b)             ((b) & 0x2)
#define BIT2(b)             ((b) & 0x4)
#define BIT3(b)             ((b) & 0x8)
#define BIT4(b)             ((b) & 0x10)
#define BIT5(b)             ((b) & 0x20)
#define BIT6(b)             ((b) & 0x40)
#define BIT7(b)             ((b) & 0x80)

#define BIT0_SET(b)         ((b) |= 0x1)
#define BIT1_SET(b)         ((b) |= 0x2)
#define BIT2_SET(b)         ((b) |= 0x4)
#define BIT3_SET(b)         ((b) |= 0x8)
#define BIT4_SET(b)         ((b) |= 0x10)
#define BIT5_SET(b)         ((b) |= 0x20)
#define BIT6_SET(b)         ((b) |= 0x40)
#define BIT7_SET(b)         ((b) |= 0x80)

#define BIT0_CLR(b)         (b &= ~0x1)
#define BIT1_CLR(b)         (b &= ~0x2)
#define BIT2_CLR(b)         (b &= ~0x4)
#define BIT3_CLR(b)         (b &= ~0x8)
#define BIT4_CLR(b)         (b &= ~0x10)
#define BIT5_CLR(b)         (b &= ~0x20)
#define BIT6_CLR(b)         (b &= ~0x40)
#define BIT7_CLR(b)         (b &= ~0x80)

#define BITN(b,n)           ((b) & (1 << n)) 
#define BIT_TEST(b,n)       ((b) & n)

/* 这里是因为Ghidra用Stackbase做 Space，offset都是负数，加了size以后，值被处理过以后不正确了 */
#define STACK_BASE          0x10000

class blockgraph;

struct VisitStat {
    SeqNum seqnum;
    int size;
};

enum height {
    a_top,
    /* 
    普通常量:
    mov r0, 1
    这样的就是普通的常量, 1是常量, 赋给r0以后，当前的r0也成了常量 */

    a_constant,
    /*
    相对常量

    0x0001. add esp, 4
    0x0002. ...
    0x0003. ...
    0x0004. sub esp, 16
    
    esp就是相对常量，我们在做一些分析的时候，会使用模拟的方式来计算esp，比如给esp一个初始值，这种方式在分析领域不能算错，
    因为比如像IDA或者Ghidra，都有对错误的容忍能力，最终的结果还是需要开发人员自己判断，但是在编译还原领域，不能有这种模糊语义
    的行为。

    假设壳的开发人员，在某个代码中采取了判断esp值的方式来实现了部分功能，比如
    if (esp > xxxxx)  {} else {}
    很可能其中一个 condition_block 就会被判断成 unreachable code。这个是给esp强行赋值带来的结果，但是不赋值可能很多计算进行不下去

    所以我们把 inst.0x0001 中的 esp设置为相对常量(rel_constant)，他的值为 esp.4, 0x0004中的esp 为  esp.16，在做常量传播时，有以下几个规则

    1. const op const = const，      (常量和常量可以互相操作，比如 加减乘除之类)
    2. sp.rel_const op const = sp.rel_const;   (非常危险，要判断这个操作不是在循环内)
    3. sp.rel_const op sp.rel_const = sp.rel_const      (这种情况起始非常少见，)
    4. sp.rel_const op rN.rel_const (top)   (不同地址的相对常量不能参与互相运算)
    */ 
    a_sp_constant,
    /* 当某个数和pc寄存器相加的时候，这个值就是pc_constant */
    a_pc_constant,
    a_bottom,

    /* */
};

class valuetype {
public:
    enum height height = a_top;

    union {
        char v1;
        short v2;
        int v4;
        intb v = 0;
    };

    int cmp(const valuetype &b) const;
    valuetype &operator=(const valuetype &op2);
    bool operator<(const valuetype &b) const { return cmp(b) < 0; }
    bool operator==(const valuetype &b) { return cmp(b) == 0;  }
    bool operator!=(const valuetype &b) { return !operator==(b); }
};

struct varnode_cmp_loc_def {
    bool operator()(const varnode *a, const varnode *b) const;
};

struct varnode_cmp_def_loc {
    bool operator()(const varnode *a, const varnode *b) const;
};

typedef set<varnode *, varnode_cmp_loc_def> varnode_loc_set;
typedef set<varnode *, varnode_cmp_def_loc> varnode_def_set;

struct pcodeop_cmp_def {
    bool operator() ( const pcodeop *a, const pcodeop *b ) const;
};

struct pcodeop_domdepth_cmp {
    bool operator() ( const pcodeop *a, const pcodeop *b ) const;
};

typedef set<pcodeop *, pcodeop_cmp_def> pcodeop_def_set;

/* 基于pcode time做的比较 */
struct pcodeop_cmp {
    bool operator() ( const pcodeop *a, const pcodeop *b ) const;
};

struct varnode_const_cmp {
    bool operator()(const varnode *a, const varnode *b) const;
};

/*

@flags  1       跳过地址比较
*/
int pcodeop_struct_cmp(pcodeop *a, pcodeop *b, uint32_t flags);

typedef map<pcodeop *, valuetype, pcodeop_cmp> valuemap;

typedef set<pcodeop *, pcodeop_cmp> pcodeop_set;

struct coverblock {
	short	version;
	short	blk_index;
	/* 这个结构主要参考自Ghidra的CoverBlock，之所以start和end，没有采用pcodeop结构是因为
	我们在优化时，会删除大量的pcodeop，这个pcode很容易失效 */
	int		start = -1;
	int		end = -1;

	coverblock() {}
	~coverblock() {}

	void set_begin(pcodeop *op);
	void set_end(pcodeop *op);
	void set_end(int);
	bool empty() {
		return (start == -1) && (end == -1);
	}

	bool contain(pcodeop *op);
	void set_all() {
		start = 0;
		end = INT_MAX;
	}
	int dump(char *buf);
};

struct cover {
	map<int, coverblock> c;

	void clear() { c.clear(); }
	void add_def_point(varnode *vn);
	void add_ref_point(pcodeop *op, varnode *vn, int exclude);
	void add_ref_recurse(flowblock *bl);
    bool contain(const pcodeop *op);
	int dump(char *buf);
};

int print_varnode(Translate *trans, char *buf, varnode *data);

class varnode {
public:
    /* varnode的值类型和值，在编译分析过后就不会被改*/
    valuetype   type;

    struct {
        unsigned    mark : 1;
        unsigned    annotation : 1;
        unsigned    input : 1;          // 没有祖先， 丄
        unsigned    written : 1;       // 是def
        unsigned    insert : 1;         // 这个
        unsigned    implied : 1;        // 是一个临时变量
        unsigned    exlicit : 1;        // 不是临时变量

        unsigned    readonly : 1;

        unsigned    covertdirty : 1;    // cover没跟新
        unsigned    virtualnode : 1;     // 虚拟节点，用来做load store分析用
        /* 一般的so中可能有类似如下的代码: 
        ldr r2, [#xxxx]
        add r2, pc
        这样写代码有一个好处就是，当so映射到内存段中以后，不需要重新修复重定向表，即可正常使用
        所以当我们发现所有值是来自于pc寄存器时，都要标注起来，任何使用pc寄存器参与的运算，都要传播开

        举例:
        r2 = r2 + pc，以后r2.from_pc = 1
        r1 = r2 + r1，以后r1.from_pc = 1
        */
        unsigned    from_pc : 1;
        /* 这个是给unique space上的临时变量用的，一般来说，临时变量只在当前block内活跃，但是有一种情况例外，
        某些单条指令内自带指向自己的branch, cbranch， 比如:
                                     - ? -
        0002574c 63 f9 0f 28     vld2.8     {d18,d19},[param_4]
                                                        $U25e0:4 = COPY 1:4
                                                        mult_addr = COPY r3
                                                        $U25e0:4 = COPY 1:4
                                                        $U3780:4 = COPY 0x390:4
                                                        $U3790:4 = INT_MULT 1:4, 8:4
                                                        $U37b0:4 = INT_ADD 0x390:4, $U3790:4
                                                        mult_dat8 = COPY 8:8
                                                      <1>
                                                        $U37c0:1 = LOAD ram(mult_addr)
                                                        STORE register($U3780:4), $U37c0:1
                                                        mult_addr = INT_ADD mult_addr, 1:4
                                                        $U37e0:1 = LOAD ram(mult_addr)
                                                        STORE register($U37b0:4), $U37e0:1
                                                        mult_addr = INT_ADD mult_addr, 1:4
                                                        mult_dat8 = INT_SUB mult_dat8, 1:8
                                                        $U3810:1 = INT_EQUAL mult_dat8, 0:8
                                                        CBRANCH <0>, $U3810:1
                                                        $U3780:4 = INT_ADD $U3780:4, 1:4
                                                        $U37b0:4 = INT_ADD $U37b0:4, 1:4
                                                        BRANCH <1>
                                                      <0>
                                                        $U25e0:4 = COPY 1:4
                                                        $U39f0:4 = COPY 2:4

        U3780和U37b0，在<1>处是入口活跃的，正常情况下要插入phi节点，但是因为我们的优化不对uniq var做，为了兼容
        这种情况，我们要标注一下这种特殊节点，强制让它生成phi
        */
        unsigned    outlive : 1;
    } flags = { 0 };

    short size = 0;
    short baseindex = 0;
    int create_index = 0;
    Address loc;

    pcodeop     *def = NULL;
    uintb       nzm;
    /* ssa的版本号，方便定位 */
    int         version = -1;

    varnode_loc_set::iterator lociter;  // sort by location
    varnode_def_set::iterator defiter;  // sort by definition

	cover				cover;
	coverblock			simple_cover;
    list<pcodeop *>     uses;    // descend, Ghidra把这个取名为descend，搞的我头晕，改成use

    varnode(int s, const Address &m);
    ~varnode();

    const Address &get_addr(void) const { return (const Address &)loc; }
    int             get_size() const { return size;  }
    intb            get_offset() { return loc.getOffset(); }
    bool            is_heritage_known(void) const { return (flags.insert | flags.annotation) || is_constant(); }
    bool            has_no_use(void) { return uses.empty(); }

    void            set_def(pcodeop *op);
    pcodeop*        get_def() { return def; }
    bool            is_constant(void) const { return type.height == a_constant; }
    bool            is_top(void) const { return type.height == a_top;  }
    bool            is_hard_constant(void) const { return (type.height == a_constant) && get_addr().isConstant(); }
    bool            in_constant_space() { return get_addr().isConstant(); }
    bool            is_pc_constant() { return type.height == a_pc_constant;  }
    bool            is_hard_pc_constant() { return flags.from_pc; }
#define MASK_SIZE(m,s)          (m & (((uintb)1 << (s * 8)) - 1))
    void            set_val(intb v) { type.height = a_constant;  type.v = v; }
    bool            in_ram();
    intb            get_ram_val() { return get_addr().getOffset();  }
    /* set_val1 会对传入的值，根据自身的size做裁剪，
    这个从语义上比set_val更加标准，也更加的正确，但是由于工程实现的原因，暂时还有以下BUG:

    c1:4 = -1;

    c1.v = 0xffffffffffffffff;
    虽然我们转成整数以后，变成8字节都是-1，但是裁剪以后
    c1.v:4 = 0x00000000ffffffff;
    变成了一个正数
    这里我们在使用一个v同时兼容 int64,32,16,8的时候，没有处理后，后期要考虑如何处理

    后面要逐渐切换到set_val1上来
    */
    void            set_val1(intb v) { 
        type.height = a_constant;  
        type.v = 0;
        switch (size) {
        case 1: type.v1 = (int8_t)v;    break;
        case 2: type.v2 = (int16_t)v;   break;
        case 4: type.v4 = (int32_t)v;   break;
        case 8: type.v = (int64_t)v;    break;
        }
    }
    void            set_top() { type.height = a_top;  }
    void            set_sub_val(int insize, intb l, intb r) {
        if (insize == 1)
            set_val((int1)l - (int1)r);
        else if (insize == 2)
            set_val((int2)l - (int2)r);
        else if (insize == 4)
            set_val((int4)l - (int4)r);
        else 
            set_val(l - r);
    }
    bool            is_sp_constant(void) { return type.height == a_sp_constant; }
    bool            is_sp_vn();
    bool            is_input(void) { return flags.input; }
    void            set_sp_constant(int v) { type.height = a_sp_constant; type.v = v;  }
    void            set_pc_constant(intb v) { type.height = a_pc_constant; type.v = v; }
    intb            get_val(void) const;

    void            add_use(pcodeop *op);
    void            del_use(pcodeop *op);
    pcodeop*        lone_use();
    bool            is_free() { return !flags.written && !flags.input; }
    /* 实现的简易版本的，判断某条指令是否在某个varnode的活跃范围内 */
    bool            in_liverange(pcodeop *p);
	bool			in_liverange_simple(pcodeop *p);
    /* 判断在某个start-end之间，这个varnode是否live, start, end必须得在同一个block内

    这2个判断liverange的代码都要重新写
    */
    bool            in_liverange(pcodeop *start, pcodeop *end);
	void			add_def_point() { cover.add_def_point(this);  }
	void			add_ref_point(pcodeop *p) { cover.add_ref_point(p, this, 0); }
	void			add_def_point_simple();
	void			add_ref_point_simple(pcodeop *p);
	void			clear_cover_simple();

    /* 给定一个varnode，搜索拷贝链，到某个OpCode类型为止，现在只搜索3种类型:

    1. copy
    2. load
    3. store

    @return NULL                失败
            ->opcode == until   成功
            ->opcode != until   失败，返回最后赋值的pcode
    */
    pcodeop*        search_copy_chain(OpCode until);
    /*

    *ptr = 0;
    label1:
    %r0 = load ptr  ; r0 = T

    store r0, xxx
    goto label1;

    本来要可以建立ptr的别名到r0，但是因为store，我们不确认%r0 是否等于 ptr，
    所以也不确认%r0的值。

    根据这个情况，我们假设:

    1. 当ptr的值为sp+c时
    
    r0不等于ptr，因为ptr假设指向的是一个sp+c的值时，它大概率是可以被计算的

    警告:这个属于pattern了，非常危险

    libmakeurl: 171fc
    */
    bool            maystore_from_this(pcodeop *maystore);

	void			clear_cover() { cover.clear();  }
	int				dump_cover(char *buf) { 
		int n = simple_cover.dump(buf);
		return n + cover.dump(buf + n);
	}
};

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
    /* 特殊操作类型里的，比如 store ,load, */
    list<pcodeop *>::iterator codeiter;
    list<pcodeop *> mayuses;

    pcodeop(int s, const SeqNum &sq);
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
    bool            all_inrefs_is_constant(void);
    bool            all_inrefs_is_adj(void);
    bool            all_inrefs_is_top(void);
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
};

typedef struct blockedge            blockedge;

#define a_tree_edge             0x1
#define a_forward_edge          0x2
#define a_cross_edge            0x4
#define a_back_edge             0x8
#define a_loop_edge             0x10
#define a_true_edge             0x20
#define a_mark                  0x40
#define a_unpropchain           0x80
#define a_do_propchain          0x100

struct blockedge {
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

/* 模拟stack行为，*/
struct mem_stack {

    int size;
    char *data;
    
    mem_stack();
    ~mem_stack();

    void    push(char *byte, int size);
    int     top(int size);
    int     pop(int size);
};

struct flowblock {
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

    /* 这个index是 反后序遍历的索引，用来计算支配节点数的时候需要用到 */
    int index = 0;
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
    bool        is_stack_check_fail();
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
    blockbasic* new_block_basic(void);
    blockbasic* new_block_basic(intb offset);

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
};

typedef struct priority_queue   priority_queue;

struct priority_queue {
    vector<vector<flowblock *> > queue;
    int curdepth;

    priority_queue(void) { curdepth = -2;  }
    void reset(int maxdepth);
    void insert(flowblock *b, int depth);
    flowblock *extract();
    bool empty(void) const { return (curdepth == -1);  }
};

typedef map<SeqNum, pcodeop *>  pcodeop_tree;
typedef struct op_edge      op_edge;
typedef struct jmptable     jmptable;

struct op_edge {
    pcodeop *from;
    pcodeop *to;
    int t = 0; // true flag

    op_edge(pcodeop *from, pcodeop *to);
    ~op_edge();
} ;

struct funcproto {
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

struct jmptable {
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

typedef funcdata* (*test_cond_inline_fn)(dobc *d, intb addr);

struct rangenode {
    intb    start = 0;
    int     size = 0;

    rangenode();
    ~rangenode();

    intb    end() { return start + size;  }
};

class pcodefunc {
public:
    funcdata *fd;
    dobc *d;
    pcodefunc(funcdata *f);
    void add_cmp_const(flowblock *b, list<pcodeop *>::iterator it, const varnode *rn, const varnode *v);
    void add__cbranch_eq(flowblock *b, int eq);
    void add_cbranch_eq(flowblock *b);
    void add_cbranch_ne(flowblock *b);
    void add_copy_const(flowblock *b, list<pcodeop *>::iterator it, const varnode &rd, const varnode &v);
};

struct ollvmhead {
    Address st1;
    int st1_size;

    Address st2;
    int st2_size;
    flowblock *h;
    int times = 0;

    ollvmhead(flowblock *h1);
    ollvmhead(Address &a, flowblock *h1);
    ollvmhead(Address &a, Address &b, flowblock *h1);
    ~ollvmhead();
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
        unsigned exit : 1;              // 有些函数有直接结束整个程序的作用，比如stack_check_fail, exit, abort
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
    } flags = { 0 };

    enum {
        a_local,
        a_global,
        a_plt,
    } symtype;

    int op_generated = 0;
	int reset_version = 0;

    pcodeop_tree     optree;
    funcproto       funcp;

    /*
    静态trace的时候，记录原始的数据值，在执行完毕后，做恢复用

    每次使用之前，必须得执行clear
    */
    valuemap   tracemap;

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
        u1      *bottom;
        u1      *top;
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
    void        dump_block(FILE *fp, blockbasic *b, int pcode);
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
    void        op_insert(pcodeop *op, blockbasic *bl, list<pcodeop *>::iterator iter);
    void        op_insert_begin(pcodeop *op, blockbasic *bl);
    void        op_insert_end(pcodeop *op, blockbasic *bl);
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
    void        rename_recurse(blockbasic *bl, variable_stack &varstack, version_map &vermap);
    /* 建立活跃链，这个活跃链非常耗时，有个 build_liverange_simple，比这个快100倍以上，假如你知道simple和这个的区别，就根据自己需要
       来使用不同的版本，假如不知道就用这个，这个就是标准的活跃链建立算法 */
	void		build_liverange();
    void        build_liverange_recurse(blockbasic *bl, variable_stack &varstack);

    int         collect(Address addr, int size, vector<varnode *> &read,
        vector<varnode *> &write, vector<varnode *> &input, int &equal);
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
    int         constant_propagation3();
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
    void        mem_rename_recurse(blockbasic *bl, variable_stack &varstack, version_map &vermap);

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
    void        set_exit(int v) { flags.exit = v; }
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
    int         ollvm_detect_propchain2(ollvmhead *oh, flowblock *&from, blockedge *&outedge, uint32_t flags);
    int         ollvm_detect_propchain3(flowblock *&from, blockedge *&outedge);
    int         ollvm_detect_propchains2(flowblock *&from, blockedge *&outedge);
    int         ollvm_detect_fsm2(ollvmhead *oh);
    int         ollvm_check_fsm(pcodeop *op);

    bool        use_outside(varnode *vn);
    void        use2undef(varnode *vn);
    void        branch_remove(blockbasic *bb, int num);
	/* 把bb和第num个节点的关系去除 */
    void        branch_remove_internal(blockbasic *bb, int num);
    void        block_remove_internal(blockbasic *bb, bool unreachable);
    bool        remove_unreachable_blocks(bool issuewarnning, bool checkexistence);
    void        splice_block_basic(blockbasic *bl);
    void        remove_empty_block(blockbasic *bl);

    void        redundbranch_apply();
    void        dump_store_info(const char *postfix);
    void        dump_load_info(const char *postfix);

    /* 循环展开时用，从start节点开始，搜索start可以到的所有节点到 end为止，全部复制出来
    最后的web包含start，不包含end */
    flowblock*  clone_web(flowblock *start, flowblock *end, vector<flowblock *> &cloneblks);
    flowblock*  clone_ifweb(flowblock *newstart, flowblock *start, flowblock *end, vector<flowblock *> &cloneblks);
	flowblock*	inline_call(pcodeop *callop, funcdata *fd);
#define F_OMIT_RETURN       1
    flowblock*  clone_block(flowblock *f, u4 flags);
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

        cfgA
        /  \
      cfgB  \
        \   /
         cfgC

    cfgA -> cfgB
    cfgA -> cfgC
    cfgB -> cfgC

    这个函数和判断是否cbranch是不一样的，那个只需要判断cfg节点是否已cbranch结尾，这个需要
    判断cfg节点出口的2个节点，能否立即在某个点汇合。
    */
    bool        is_ifthenfi_structure(flowblock *a, flowblock *b, flowblock *c);
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
};

struct func_call_specs {
    pcodeop *op;
    funcdata *fd;

    func_call_specs(pcodeop *o, funcdata *f);
    ~func_call_specs();

    const string &get_name(void) { return fd->name;  }
    const Address &get_addr() { return fd->get_addr(); }
};

class dobc:public AddrSpaceManager {
public:
    string  archid;

    /* 代码生成以后需要被覆盖的loader */
    ElfLoadImage *loader;
    /* 原始加载的elf文件 */
    ElfLoadImage *loader1;
    string slafilename;

    string fullpath;
    string filename;

    /* 输出目录，一般是输出到文件所在的目录，假如开启dc选项，则是开启到 */
    string out_dir;
    /* 输出文件名，默认是 xxx.decode.so */
    string out_filename;

    ContextDatabase *context = NULL;
    Translate *trans = NULL;

    map<Address, funcdata *> addrtab;
    map<string, funcdata *> nametab;

#define SHELL_OLLVM           0
#define SHELL_360FREE         1
    int shelltype = -1;

    int max_basetype_size;
    int min_funcsymbol_size;
    int max_instructions;
    map<string, string>     abbrev;
    test_cond_inline_fn test_cond_inline = NULL;

    intb    stack_check_fail_addr;

    Address     sp_addr;
    Address     r0_addr;
    Address     r1_addr;
    Address     r2_addr;
    Address     r3_addr;
    Address     r4_addr;
    Address     r5_addr;
    Address     r6_addr;
    Address     r7_addr;
    Address     r8_addr;
    Address     r9_addr;
    Address     r10_addr;
    Address     r11_addr;
    Address     ma_addr;

    Address     lr_addr;
    Address     zr_addr;
    Address     cy_addr;
    Address     pc_addr;
    set<Address> cpu_regs;
	/* r0-sp */
    set<Address> liveout_regs;
    vector<Address *>   argument_regs;

    AddrSpace *ram_spc = NULL;
    AddrSpace *reg_spc = NULL;

    /* 记录全部指令的助记符(mnemonic)，用来确认指令类型用的 
    
    WARN & TODO:在做了去壳和重新生成exe写入以后，具体地址的指令类型发生了变化，这个时候要
                跟新这个表和funcdata，不过现在没做。
    */
    map<intb, string> mnemtab;

    int wordsize = 4;
    vector<int>     insts;
    vector<string>  useroplist;

    dobc(const char *slafilename, const char *filename);
    ~dobc();
    static dobc*    singleton();

    void init(DocumentStorage &store);
    void init_regs();
    void init_spcs();
    /* 初始化位置位置无关代码，主要时分析原型 */
    void init_plt(void);

    void        add_inst_mnem(const Address &addr, const string &mnem);
    string&     get_inst_mnem(intb addr);
    funcdata*   add_func(const Address &addr);
    string&     get_userop_name(int i) { return useroplist[i];  }
    void        set_shelltype(char *shelltype);

#define strprefix(m1,c)     (strncmp(m1.c_str(), c, strlen(c)) == 0)
    bool        is_simd(const Address &addr) { return context->getVariable("simd", addr);  }

    /* 当我们重新生成了新的obj文件以后，原始的context里面还遗留了以前的context信息，比如it block的信息，像
    
    condit
    itmod
    cond_full
    cond_base
    cond_true
    cond_shft
    cond_mask
    我们会清掉一部分，否则重新写下去的代码，解析不正确
    */
    void        clear_it_info(const Address &addr) {
        context->setVariable("condit", addr, 0);
    }
    bool        is_adr(const Address &addr) { 
        string &m = get_inst_mnem(addr.getOffset());
        return strprefix(m, "adr");
    }
    /* 使用strprefix来判断指令的前缀时 */
    bool        is_vld(const Address &addr) {
        string &m = get_inst_mnem(addr.getOffset());
        return strprefix(m, "vld") && (m[3] >= '1' && m[3] <= '3');
    }
    bool        is_vldr(const Address &addr) { 
        string &m = get_inst_mnem(addr.getOffset());
        return strprefix(m, "vldr");
    }

    int         func_is_thumb(int offset);
    void        run();
    void        set_func_alias(const string &func, const string &alias);
    void        set_test_cond_inline_fn(test_cond_inline_fn fn1) { test_cond_inline = fn1;  }
    funcdata*   find_func(const string &s);
    funcdata*   find_func(const Address &addr);
    bool        is_liveout_regs(const Address &addr) {
        return is_greg(addr) || is_vreg(addr);
    }
    bool        is_cpu_reg(const Address &addr) { return cpu_regs.find(addr) != cpu_regs.end();  }
    /* vector variable */
    bool        is_vreg(const Address &addr) { 
        return  trans->getRegister("d0").getAddr() <= addr && addr <= trans->getRegister("d31").getAddr();  
    }
    Address     get_addr(const string &name) { return trans->getRegister(name).getAddr();  }
    bool        is_temp(const Address &addr) { return addr.getSpace() == trans->getUniqueSpace();  }
    /* temp status register, tmpNG, tmpZR, tmpCY, tmpOV, */
    bool        is_tsreg(const Address &addr) { return trans->getRegister("tmpNG").getAddr() <= addr && addr <= trans->getRegister("tmpOV").getAddr();  }
    /* status register */
    bool        is_sreg(const Address &addr) { return trans->getRegister("NG").getAddr() <= addr && addr <= trans->getRegister("OV").getAddr();  }
    /* is general purpose register */
    bool        is_greg(const Address &addr) { return (get_addr("r0") <= addr) && (addr <= get_addr("pc"));  }
    int         reg2i(const Address &addr);
    int         vreg2i(const Address &addr);
    Address     i2reg(int i);
    /* 获取经过call被破坏的寄存器，依赖于CPU架构 */
    void        get_scratch_regs(vector<int> &regs);

    void    plugin_dvmp360();
    void    plugin_ollvm();

    void gen_sh(void);
    void init_abbrev();
    const string &get_abbrev(const string &name);

    void add_space_base(AddrSpace *basespace,
        const string &nm, const VarnodeData &ptrdata, int trunsize, bool isreversejustified, bool stackGrowth);

    void        build_loader(DocumentStorage &store);
    void        build_context();
    Translate*  build_translator(DocumentStorage &store);
    void        build_arm();

    void restore_from_spec(DocumentStorage &storage);
    void parse_stack_pointer(const Element *el);

    void parseCompilerConfig(DocumentStorage &store);
    AddrSpace *getStackBaseSpace(void) {
        return getSpaceByName("stack");
    }
    AddrSpace *getSpaceBySpacebase(const Address &loc,int4 size) const; ///< Get space associated with a \e spacebase register
};


#endif

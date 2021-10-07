
#ifndef __varnode_hh__
#define __varnode_hh__

#include "types.h"
#include "opcodes.hh"
#include "address.hh"
#include "cover.hh"

/* 这里是因为Ghidra用Stackbase做 Space，offset都是负数，加了size以后，值被处理过以后不正确了 */
#define STACK_BASE          0x10000

class funcdata;

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
    /* 虽然值不可计算，但是知道是 偶数，比如 x * (x - 1)  */
    a_top_even,
    /* 虽然值不可计算，但是知道是 奇数 x * (x - 1) ^ 0xfffffffe */
    a_top_odd,
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

class pcodeop;

struct varnode_cmp_loc_def {
    bool operator()(const varnode *a, const varnode *b) const;
};

struct varnode_cmp_def_loc {
    bool operator()(const varnode *a, const varnode *b) const;
};

typedef set<varnode *, varnode_cmp_loc_def> varnode_loc_set;
typedef set<varnode *, varnode_cmp_def_loc> varnode_def_set;

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

    /* 尝试收集这个变量的所有常量定义，
    
    这个值表，每次随着heritage重新生成
    */
    vector<varnode* >   const_defs;
    /* 补充上面的const_defs，判断是否有无法分析的值 */
    vector<varnode* >   top_defs;
    /* 这个版本号，是用来判断const_defs里的值是否符合最新的heritage，假如不是就要清空 */
    int heritage_ver = 0;
    funcdata *fd = NULL;

    varnode(funcdata *fd, int s, const Address &m);
    ~varnode();

    const Address &get_addr(void) const { return (const Address &)loc; }
    int             get_size() const { return size;  }
    intb            get_offset() { return loc.getOffset(); }
    bool            is_heritage_known(void) const { return (flags.insert | flags.annotation) || is_constant(); }
    bool            has_no_use(void) { return uses.empty(); }
    bool            has_use(void) { return uses.size() > 0; }

    void            set_def(pcodeop *op);
    pcodeop*        get_def() { return def; }
    bool            is_constant(void) const { return type.height == a_constant; }
    /* 判断是否是一种更通用的常量形式*/
    bool            is_gen_constant(void) const { return type.height == a_constant || type.height == a_sp_constant || type.height == a_pc_constant;  }
    /* 判断是否时简单常数 ，这个主要是勇于判断当我们在ollvm做循环展开时，是在展开一个普通的循环还是ollvm循环
    */
    bool            is_simple_constant(void) const { return is_constant() && (get_val() >= 0 ) && (get_val() < 1024); }
    bool            is_top(void) const { return (type.height == a_top) || (type.height == a_top_even) || (type.height == a_top_odd);  }
    bool            is_top_even(void) const { return type.height == a_top_even;  }
    bool            is_top_odd(void) const { return type.height == a_top_odd;  }
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
    void            set_top_even() { type.height = a_top_even;  }
    void            set_top_odd() { type.height = a_top_odd;  }
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
    bool            is_sp_vn() const;
    bool            is_input(void) { return flags.input; }
    void            set_sp_constant(int v) { type.height = a_sp_constant; type.v = v;  }
    void            set_pc_constant(intb v) { type.height = a_pc_constant; type.v = v; }
    intb            get_val(void) const;
    bool            is_val(intb v) {
        return is_constant() && (get_val() == v);
    }
    bool            is_val_even() {
        return is_constant() && ((get_val() & 1) == 0);
    }
    bool            is_val_odd() {
        return is_constant() && ((get_val() & 1) == 1);
    }
    intb            get_sp_offset() { return get_addr().getOffset() - STACK_BASE; }

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
    pcodeop*        search_copy_chain(OpCode until, flowblock *until_blk);
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

    void            collect_all_const_defs();
};


#endif
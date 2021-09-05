

#ifndef __dobc_h__
#define __dobc_h__

#include "mcore/mcore.h"
#include "elfloadimage.hh"
#include "types.h"
#include "heritage.hh"
#include "pcodefunc.hh"
#include "pcodeop.hh"
#include "block.hh"
#include "funcdata.hh"
#include <bitset>

class dobc;

/* 重命名一些符号，做内部使用，我懒的改opcodes.hh了 */
/*
这个use是为了解决这么一个问题:

假设我们需要把一个stack上的变量和某个寄存器比较

2. - 0x10: cmp r0, [sp + 3a]

这里，我们的sp，其实代表的不是当前的sp的值，而是基准sp寄存器，当前的sp值
其实是前面的 sp - 0x10

所以最后指令2，其实需要转换为当前sp的值，减去操作数里sp地址的值

最后的代码生成时，应该是:

rm = regalloc();
mov rm, [sp + 4a]
cmp r0, rm
*/
#define CPUI_USE        CPUI_CAST

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

class blockgraph;


struct pcodeop_domdepth_cmp {
    bool operator() ( const pcodeop *a, const pcodeop *b ) const;
};


/* 基于pcode time做的比较 */
struct varnode_const_cmp {
    bool operator()(const varnode *a, const varnode *b) const;
};

/*

@flags  1       跳过地址比较
*/
int pcodeop_struct_cmp(pcodeop *a, pcodeop *b, uint32_t flags);


int print_varnode(Translate *trans, char *buf, varnode *data);
int print_udchain(char *buf, pcodeop *op, uint32_t flags);


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

class func_call_specs {
public:
    pcodeop *op;
    funcdata *fd;

    func_call_specs(pcodeop *o, funcdata *f);
    ~func_call_specs();

    const string &get_name(void) { return fd->name;  }
    const Address &get_addr() { return fd->get_addr(); }
};



typedef funcdata* (*test_cond_inline_fn)(dobc *d, intb addr);


class ollvmhead {
public:
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


#define ARM_SLA            "/Processors/ARM/data/languages/ARM8_le.sla"

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
    string ghidra;

    /* 输出目录，一般是输出到文件所在的目录，假如开启dc选项，则是开启到 */
    string out_dir;
    /* 输出文件名，默认是 xxx.decode.so */
    string out_filename;

    ContextDatabase *context = NULL;
    Translate *trans = NULL;

    map<Address, funcdata *> addrtab;
    map<string, funcdata *> nametab;

    vector<intb>    decode_address_list;
    vector<string>  decode_symbol_list;
    vector<intb>    noreturn_calls;

#define SHELL_OLLVM           0
#define SHELL_360FREE         1
    int shelltype = SHELL_OLLVM;

    int max_basetype_size;
    int min_funcsymbol_size;
    int max_instructions;
    map<string, string>     abbrev;
    test_cond_inline_fn test_cond_inline = NULL;

    struct {
        /* 是否打印dfa_connect中的cfg流图 */
        int dump_cfg;
        int level;

        /* refer to main.cc:help */
        int dump_inst0;
        int dump_inst1;     
        int dump_inst2;
        int open_phi2;
    } debug = { 0 };

    /* 特殊用，创建phi的时候，默认地址 */
    Address     zero_addr;

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
    Address     ng_addr;
    Address     ov_addr;
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

    set<string>     noreturn_func_tab;
    string          noreturn_elf;
    string          noreturn_xmach;
    string          noreturn_mach;
    string          noreturn_pe;

    dobc();
    ~dobc();
    static dobc*    singleton();

    void init(DocumentStorage &store);
    void init_regs();
    void init_spcs();
    /* 初始化位置位置无关代码，主要时分析原型 */
    void init_plt(void);

    /* 初始化符号，包括库里加载的符号，外部指定的noreturn符号等等 */
    void init_syms();

    void        add_inst_mnem(const Address &addr, const string &mnem);
    string&     get_inst_mnem(intb addr);
    void        add_func(funcdata *fd);
    funcdata*   add_func(const string &name);
    funcdata*   add_func(const Address &addr);
    string&     get_userop_name(int i) { return useroplist[i];  }
    void        set_shelltype(char *shelltype);
    void        set_ghidra(const char *ghidra_path);
    void        set_input_bin(const char *bin_path);
    void        set_output_dir(string &out) {
        out_dir = out;
    }
    void        set_output_filename(const char *filename) {
        out_filename.assign(filename);
    }

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

    void        build_noreturn_function();
    void        build_function_type();

    void restore_from_spec(DocumentStorage &storage);
    void parse_stack_pointer(const Element *el);

    void parseCompilerConfig(DocumentStorage &store);
    AddrSpace *getStackBaseSpace(void) {
        return getSpaceByName("stack");
    }
    AddrSpace *getSpaceBySpacebase(const Address &loc,int4 size) const; ///< Get space associated with a \e spacebase register
};


#endif

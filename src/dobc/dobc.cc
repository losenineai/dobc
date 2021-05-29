

//#include "sleigh_arch.hh"
#include "sleigh.hh"
#include "dobc.hh"
#include "thumb_gen.hh"
#include <iostream>
#include <assert.h>
#include "vm.h"

#define strdup _strdup 

#define NOP             0xe1a00000

#define GEN_SH          \
    "#!/bin/bash\n" \
    "if [[ $#  -eq 1 ]]\n" \
    "then\n" \
    "  filename=$1\n" \
    "  echo `date +\"%T.%3N\"` gen $filename svg\n" \
    "  dot -Tsvg -o ${filename%.*}.svg $1\n" \
    "else\n" \
    "  for filename in `find . - type f - name \"*.dot\" | xargs`\n" \
    "  do\n" \
    "    echo `date + \"%T.%3N\"` gen $filename svg\n" \
    "    dot -Tsvg -o ${filename%.*}.svg $filename\n" \
    "  done\n" \
    "fi\n"

//#define ENABLE_DUMP_INST                1
//#define ENABLE_DUMP_PCODE               1

#define func_format_s					"%s"
#define func_format()					""
#undef print_level
#define print_level		3

static dobc *g_dobc = NULL;

#if 1
#define DCFG_COND_INLINE                
#define DCFG_BEFORE               
#define DCFG_AFTER               
#define DCFG_AFTER_PCODE        
#define DCFG_CASE
#endif

#define MSB4(a)                 (a & 0x80000000)
#define MSB2(a)                 (a & 0x8000)

static void print_vardata(Translate *trans, FILE *fp, VarnodeData &data)
{
    string name = trans->getRegisterName(data.space, data.offset, data.size);

    if (name == "")
        fprintf(fp, "(%s,0x%llx,%d)", data.space->getName().c_str(), data.offset, data.size);
    else
        fprintf(fp, "(%s,%s,%d)", data.space->getName().c_str(), name.c_str(), data.size);
}

int print_vartype(Translate *trans, char *buf, varnode *data)
{
    if (!data) {
        return sprintf(buf, " ");
    }

    if (data->type.height == a_constant) {
        if (data->size == 8)
            return sprintf(buf, "%llx", data->type.v);
        else 
            return sprintf(buf, "%x", (u4)data->type.v);
    }
    else if (data->type.height == a_sp_constant) {
        string name = "sp";
        Address &addr = trans->getRegister(name).getAddr();
        name = g_dobc->get_abbrev(name);

        return sprintf(buf, "%c%s%c%llx", addr.getShortcut(), name.c_str(), data->type.v > 0 ? '+':'-', abs(data->type.v));
    }
    else if (data->type.height == a_pc_constant) {
        return sprintf(buf, "z%c%llx", data->type.v > 0 ? '+':'-', abs(data->type.v));
    }
    else 
        return sprintf(buf, "T");
}

static int print_udchain(char *buf, pcodeop *op, uint32_t flags)
{
    varnode *out = op->output;
    int i = 0, j, defs = 0, uses_limit = 10000, defs_limit = 10000;

    if (flags & PCODE_OMIT_MORE_USE)
        uses_limit = 7;

    if (out && out->uses.size()) {
        list<pcodeop *>::iterator iter = out->uses.begin();
        i += sprintf(buf + i, " [u:");
        for (j = 0; iter != out->uses.end(); iter++, j++) {
            /* 最多打印limit个use */
            if (j == uses_limit)
                break;

            i += sprintf(buf + i, "%d ", (*iter)->start.getTime());
        }
        if (iter != out->uses.end())
        /* 遇见没打印完的use，以省略号代替，*/
            i += sprintf(buf + i, "...]");
        else {
            /* 删除末尾多余的空格 */
            if (j > 0) i--;

            i += sprintf(buf + i, "]");
        }
    }

    for (j = 0; j < op->inrefs.size(); j++) {
        if (op->inrefs[j]->def)
            defs++;
    }

    if (defs) {
        if (flags & PCODE_OMIT_MORE_DEF)
            defs_limit = 7;

        i += sprintf(buf + i, " [d:");
        for (j = 0; j < op->inrefs.size(); j++) {
            if (j == defs_limit) break;

            if (op->inrefs[j]->def)
                i += sprintf(buf + i, "%d ", op->inrefs[j]->def->start.getTime());
        }

        if (j == defs_limit)
            i += sprintf(buf + i, "...]");
        else {
            if (j > 0) i--;
            i += sprintf(buf + i, "]");
        }
    }

    return i;
}

int print_varnode(Translate *trans, char *buf, varnode *data)
{
    Address addr = data->get_addr();
    string name = trans->getRegisterName(addr.getSpace(), addr.getOffset(), data->size);

    name = g_dobc->get_abbrev(name);

    /* 有可能常数会撞上? */
    if ((addr.isConstant()) && ((intb)addr.getOffset() == (intb)trans->getDefaultCodeSpace()))
        return sprintf(buf, "ram");
    else if ((addr.isConstant()) && ((intb)addr.getOffset() == (intb)g_dobc->reg_spc))
        return sprintf(buf, "reg");
    else if (name == "") {
        if (addr.getSpace()->getIndex() == IPTR_CONSTANT)
            return sprintf(buf, "(%c%llx:%d)", addr.getSpace()->getShortcut(), addr.getOffset(), data->size);
        else
            return sprintf(buf, "(%c%llx:%d)", addr.getSpace()->getShortcut(), addr.getOffset(), data->size);
    }
    else
        return sprintf(buf, "(%c%s.%d:%d)", addr.getSpace()->getShortcut(), name.c_str(), data->version, data->size);
}

void pcodeemit2::dump(const Address &addr, OpCode opc, VarnodeData *outvar, VarnodeData *vars, int4 isize)
{
    int i;
    pcodeop *op;
    varnode *vn;
    dobc *d = fd->d;

    if (outvar != (VarnodeData *)0) {
        Address oaddr(outvar->space, outvar->offset);
        op = fd->newop(isize, addr);
        fd->new_varnode_out(outvar->size, oaddr, op);
    }
    else
        op = fd->newop(isize, addr);

    fd->op_set_opcode(op, opc);

    op->flags.itblock = itblock;

    i = 0;
    if (op->is_coderef()) {
        Address addrcode(vars[0].space, vars[0].offset);
        fd->op_set_input(op, fd->new_coderef(addrcode), 0);
        i++;
    }

    for (; i < isize; i++) {
        /* 任何有pc寄存器参与的运算，所有的值转换为ram constant*/
        vn = fd->new_varnode(vars[i].size, vars[i].space, vars[i].offset);
        fd->op_set_input(op, vn, i);

        if (vn->is_constant() && vn->get_offset() == addr.getOffset()) {
            vn->set_pc_constant(vn->get_offset());
            vn->flags.from_pc = 1;
        }
    }

    if (d->is_adr(addr)) {
        vn->set_pc_constant(vn->get_offset());
        vn->flags.from_pc = 1;
    }

    prevp = op;

#if ENABLE_DUMP_PCODE
    fprintf(fp, "    ");

    if (outvar) {
        print_vardata(fd->d->trans, fp, *outvar); fprintf(fp, " = ");
    }

    fprintf(fp, "%s", get_opname(opc));
    // Possibly check for a code reference or a space reference
    for (i = 0; i < isize; ++i) {
        fprintf(fp, " ");
        print_vardata(fd->d->trans, fp, vars[i]);
    }
    fprintf(fp, "\n");
#endif

}

#define zext(in,insiz,outsiz)               in

intb sext(intb in, int insiz, int outsz)
{
    if (in & ((intb)1 << (insiz - 1)))
        return (~(((intb)1 << insiz) - 1)) | in;

    return in;
}

int valuetype::cmp(const valuetype &b) const
{
    if (height == b.height) {
        /* a_top代表的是 unknown或者undefined，这个值无法做比较， 例如数里面的 无限大 和 无限大 无法做比较。
        他们都是不相等
        */
        if (height == a_top) return 1;

        return v - b.v;
    }

    return height - b.height;
}

valuetype &valuetype::operator=(const valuetype &op2)
{
    height = op2.height;
    v = op2.v;

    return *this;
}

int         dobc::func_is_thumb(int offset)
{
    return 1;
}

// FIXME:loader的类型本来应该是LoadImageB的，但是不知道为什么loader的getNextSymbol访问的是
// LoadImageB的，而不是ElfLoadImage的···
void dobc::run()
{
    if (shelltype == SHELL_OLLVM)
        plugin_ollvm();
    else if (shelltype == SHELL_360FREE)
        plugin_dvmp360();
}

void dobc::init_abbrev()
{
    abbrev["mult_addr"] = "ma";
    abbrev["shift_carry"] = "sh_ca";
}

struct pltentry {
    char *name;
    intb addr;
    int input;
    int output;
    int exit;
    int side_effect;
    u4 flags;
} pltlist[] = {
    { "__stack_chk_fail",   0x1a34, 4, 0, 1, 0 },
    { "time",               0x1adc, 4, 1, 0, 0 },
    { "lrand48",            0x1ad0, 4, 1, 0, 0 },
    { "srand48",            0x1ac4, 4, 0, 0, 0 },
    { "memcpy",             0x1ab8, 4, 1, 0, 1 },
    { "dec_str",            0x7dfc, 4, 1, 0, 1 },
    { "anti1",              0x1d68, 4, 1, 0, 0 },
    { "anti2",              0x2000, 4, 1, 0, 0 },
    { "vmp360_op1",         0x67ac, 4, 0, 0, 1 },
    { "vmp360_mathop",      0x6248, 4, 0, 0, 1 },
    { "vmp360_op3",         0x66ac, 4, 0, 0, 1 },
    { "vmp360_op4",         0x61c0, 4, 0, 0, 1 },
    { "vmp360_op5",         0x6204, 4, 0, 0, 1 },
    { "vmp360_cbranch",     0x68cc, 4, 0, 0, 1 },
};

void        dobc::add_inst_mnem(const Address &addr, const string &mnem)
{
    mnemtab[addr.getOffset()] = mnem;
}

string&     dobc::get_inst_mnem(intb addr)
{
    static string empty = "";

    auto it = mnemtab.find(addr);
    if (it == mnemtab.end())
        return empty;

    return it->second;
}

funcdata *dobc::add_func(const Address &a)
{
    char buf[128];
    funcdata *fd = NULL;
    if ((fd = find_func(a))) return fd;

    if (a.getSpace() == ram_spc) {
        Address addr(trans->getDefaultCodeSpace(), a.getOffset());

        sprintf(buf, "sub_%llx", a.getOffset());
        fd = new funcdata(buf, addr, 0, this);
    }
    else if (a.getSpace() == trans->getDefaultCodeSpace()) {
        sprintf(buf, "sub_%llx", a.getOffset());
        fd = new funcdata(buf, a, 0, this);
    }

    return fd;
}

void dobc::init_plt()
{
    funcdata *fd;
    int i;

    if (shelltype == SHELL_360FREE) {
        for (i = 0; i < count_of_array(pltlist); i++) {
            struct pltentry *entry = &pltlist[i];

            Address addr(trans->getDefaultCodeSpace(), entry->addr);
            fd = new funcdata(entry->name, addr, 0, this);
            fd->set_exit(entry->exit);
            fd->funcp.set_side_effect(entry->side_effect);
            fd->funcp.inputs = entry->input;
            fd->funcp.output = 1;
            addrtab[addr] = fd;
        }
    }
    else {
        Address addr(trans->getDefaultCodeSpace(), stack_check_fail_addr);
        fd = new funcdata("__stack_check_fail", addr, 0, this);
        fd->set_exit(1);
        addrtab[addr] = fd;
        nametab[fd->name] = fd;
    }
}

void        dobc::set_shelltype(char *st)
{
    if (!strcmp(st, "ollvm"))
        shelltype = SHELL_OLLVM;
    else if (!strcmp(st, "360free"))
        shelltype = SHELL_360FREE;
    else
        throw LowlevelError("not support shell type" );
}

const string& dobc::get_abbrev(const string &name)
{
    map<string, string>::iterator iter = abbrev.find(name);

    return (iter != abbrev.end()) ? (*iter).second : name;
}

funcdata* test_vmp360_cond_inline(dobc *d, intb addr)
{
    int i;

    for (i = 0; i < count_of_array(pltlist); i++) {
        pltentry *e = pltlist + i;
        if ((e->addr == addr) && strstr(e->name, "vmp360")) {
            uintb uaddr = (uintb)addr;
            Address addr(d->get_code_space(), uaddr);
            return d->find_func(addr);
        }
    }

    return NULL;
}

void dobc::plugin_ollvm()
{
#if 0
    funcdata *fd_main = find_func(std::string("JNI_OnLoad"));
    //funcdata *fd_main = find_func(Address(trans->getDefaultCodeSpace(), 0x407d));
    //funcdata *fd_main = find_func(Address(trans->getDefaultCodeSpace(), 0x367d));
#else
    //funcdata *fd_main = add_func(Address(trans->getDefaultCodeSpace(), 0x15521));
    //funcdata *fd_main = add_func(Address(trans->getDefaultCodeSpace(), 0x366f5));

    //funcdata *fd_main = add_func(Address(trans->getDefaultCodeSpace(), 0x15f09));
    funcdata *fd_main = add_func(Address(trans->getDefaultCodeSpace(), 0x132ed));
#endif
    fd_main->ollvm_deshell();
    loader->saveFile("test.so");
}

void dobc::plugin_dvmp360()
{
    funcdata *fd_main = find_func(std::string("_Z10__arm_a_21v"));
    //funcdata *fd_main = find_func("_Z9__arm_a_1P7_JavaVMP7_JNIEnvPvRi");
    //funcdata *fd_main = find_func("_Z9__arm_a_2PcjS_Rii");
    //funcdata *fd_main = find_func("_ZN10DynCryptor9__arm_c_0Ev");
    //funcdata *fd_main = find_func("_ZN9__arm_c_19__arm_c_0Ev");
    fd_main->set_alias("vm_func1");

    set_func_alias("_Z9__arm_a_0v", "vm_enter");
    set_func_alias("_Z10__fun_a_18Pcj", "vm_run");

    set_test_cond_inline_fn(test_vmp360_cond_inline);

    fd_main->vmp360_deshell();
}

void    funcdata::vmp360_marker(pcodeop *p)
{
    if (p->opcode == CPUI_LOAD) {
        varnode *in1 = p->get_in(1);

        if ((in1->type.height == a_sp_constant) && (in1->get_val() == vmeip)) {
            p->flags.vm_eip = 1;
        }
    }
    else if (p->opcode == CPUI_STORE) {
        varnode *in1 = p->get_in(1);
        varnode *in2 = p->get_in(2);

        /* 加入跟新 VMEIP 指针的值不是常量直接报错 */
        if ((in1->type.height == a_sp_constant) && (in1->get_val() == vmeip)) {
            p->flags.vm_eip = 1;
            if (!p->flags.vm_vis && (in2->type.height == a_constant)) {
                //printf("addr = %llx, p%d, store VMEIP = %lld\n", p->get_dis_addr().getOffset(), p->start.getTime(), in2->get_val());
                p->flags.vm_vis = 1;
            }
        }
    }
}

funcdata* dobc::find_func(const Address &addr)
{
    map<Address, funcdata *>::iterator it;

    it = addrtab.find(addr);
    return (it != addrtab.end()) ? it->second : NULL;
}

funcdata* dobc::find_func(const string &s)
{
    map<string, funcdata *>::iterator it;

    it = nametab.find(s);

    return (it != nametab.end()) ? it->second:NULL;
}

void dobc::init_spcs()
{
    for (int i = 0; i < trans->numSpaces(); i++) {
        AddrSpace *spc = trans->getSpace(i);
        if (spc->getName() == "ram")
            ram_spc = spc;
        else if (spc->getName() == "register")
            reg_spc = spc;
    }
}

void dobc::init_regs()
{
    map<VarnodeData, string> reglist;
    map<VarnodeData, string>::iterator it;

    trans->getAllRegisters(reglist);

    for (it = reglist.begin(); it != reglist.end(); it++) {
        string &name = it->second;
        if (name.find("tmp") != string::npos) continue;
        if (name.find("multi") != string::npos) continue;
        cpu_regs.insert(it->first.getAddr());
    }
}

void dobc::build_instructions()
{
}

void dobc::init()
{
    init_spcs();
    init_regs();
    init_abbrev();
    init_plt();
    build_instructions();

    addrtab::iterator it;
    LoadImageFunc *sym;
    funcdata *fd;

    for (it = loader->beginSymbol(); it != loader->endSymbol(); it++) {
        sym = it->second;

        if (NULL == (fd  = find_func(sym->address)))
            fd = new funcdata(sym->name.c_str(), sym->address, sym->size, this);

        addrtab[sym->address] = fd;
        nametab[sym->name] = fd;
    }
}

void dobc::gen_sh(void)
{
    char buf[260];

    sprintf(buf, "%s/gen.sh", filename.c_str());
    file_save(buf, GEN_SH, strlen(GEN_SH));
}

dobc::dobc(const char *sla, const char *bin) 
    : fullpath(bin)
{
    g_dobc = this;

    slafilename.assign(sla);
    filename.assign(basename(bin));

    loader = new ElfLoadImage(bin);
    loader1 = new ElfLoadImage(bin);
    context = new ContextInternal();
    trans = new Sleigh(loader, context);

    DocumentStorage docstorage;
    Element *sleighroot = docstorage.openDocument(slafilename)->getRoot();
    docstorage.registerTag(sleighroot);
    trans->initialize(docstorage); // Initialize the translator
    // 开启这一行，会导致it指令的上下切换出错
    //trans->allowContextSet(false); 
    trans->setContextDefault("TMode", 1);
    //trans->setContextDefault("LRset", 0);

    loader->setCodeSpace(trans->getDefaultCodeSpace());
    loader->init();

    mdir_make(filename.c_str());
    gen_sh();

    sp_addr = trans->getRegister("sp").getAddr();
    lr_addr = trans->getRegister("lr").getAddr();
    r0_addr = trans->getRegister("r0").getAddr();
    r1_addr = trans->getRegister("r1").getAddr();
    r2_addr = trans->getRegister("r2").getAddr();
    r3_addr = trans->getRegister("r3").getAddr();
    r4_addr = trans->getRegister("r4").getAddr();
    r5_addr = trans->getRegister("r5").getAddr();
    r6_addr = trans->getRegister("r6").getAddr();
    r7_addr = trans->getRegister("r7").getAddr();
    r8_addr = trans->getRegister("r8").getAddr();
    r9_addr = trans->getRegister("r9").getAddr();
    r10_addr = trans->getRegister("r10").getAddr();
    r11_addr = trans->getRegister("r11").getAddr();
    ma_addr = trans->getRegister("mult_addr").getAddr();

    zr_addr = trans->getRegister("ZR").getAddr();
    cy_addr = trans->getRegister("CY").getAddr();
    pc_addr = trans->getRegister("pc").getAddr();

    argument_regs.push_back(&r0_addr);
    argument_regs.push_back(&r1_addr);
    argument_regs.push_back(&r2_addr);
    argument_regs.push_back(&r3_addr);

}

dobc*    dobc::singleton()
{
    return g_dobc;
}

dobc::~dobc()
{
}

void        dobc::set_func_alias(const string &sym, const string &alias)
{
    funcdata *fd = find_func(sym);

    fd->set_alias(alias);
}

#define SLA_FILE            "../../../Processors/ARM/data/languages/ARM8_le.sla"
#define PSPEC_FILE          "../../../Processors/ARM/data/languages/ARMCortex.pspec"
#define CSPEC_FILE          "../../../Processors/ARM/data/languages/ARM.cspec"
#define TEST_SO             "../../../data/vmp/360_1/libjiagu.so"

static char help[] = {
    "dobc [-s .sla filename] [-st (360free|ollvm)] [-i filename] [-stack_check_fail addr]"
};

#if defined(DOBC)
int main(int argc, char **argv)
{
    int i;
    char *sla = NULL, *filename = NULL, *st = NULL;
    intb stack_check_fail_addr = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            sla = argv[++i];
        }
        else if (!strcmp(argv[i], "-st")) {
            st = argv[++i];
        }
        else if (!strcmp(argv[i], "-i")) {
            filename = argv[++i];
        }
        else if (!strcmp(argv[i], "-stack_check_fail")) {
            stack_check_fail_addr = strtol(argv[++i], NULL, 16);
        }
    }

    if (!sla || !st || !filename || !stack_check_fail_addr) {
        printf("argument error\n");
        puts(help);
        return -1;
    }

    dobc d(sla, filename);

    d.set_shelltype(st);
    d.stack_check_fail_addr = stack_check_fail_addr;


    d.init();
    d.run();

    return 0;
}

#endif

varnode::varnode(int s, const Address &m)
    : loc(m)
{
    if (!m.getSpace())
        return;

    size = s;

    spacetype tp = m.getSpace()->getType();

    type.height = a_top;

    if (tp == IPTR_CONSTANT) {
        nzm = m.getOffset();

        set_val(m.getOffset());
    }
    else if ((tp == IPTR_FSPEC) || (tp == IPTR_IOP)) {
        flags.annotation = 1;
        flags.covertdirty = 1;
        nzm = ~((uintb)0);
    }
    else {
        flags.covertdirty = 1;
        nzm = ~((uintb)0);
    }

}

varnode::~varnode()
{
}

bool            varnode::in_ram() 
{ 
    return get_addr().getSpace() == dobc::singleton()->ram_spc; 
}

void            varnode::set_def(pcodeop *op)
{
    def = op;
    if (op) {
        flags.written = 1;
    }
    else
        flags.written = 0;
}

void        funcdata::dump_phi_placement(int bid, int pid)
{
    flowblock *b = bblocks.get_block_by_index(bid);
    pcodeop *p = b->get_pcode(pid);

    if (!p) return;

    vector<varnode *> writevars;

    writevars.push_back(p->output);

    calc_phi_placement(writevars);
    printf("p%d merge point:", p->start.getTime());
    for (int i = 0; i < merge.size(); i++) {
        printf("%d ", merge[i]->index);
    }
    printf("\n");
}

bool        funcdata::is_out_live(pcodeop *op)
{
    pcodeop_def_set::iterator it = topname.find(op);
    if ((it != topname.end()))
        return true;

    return false;
}

varnode*    funcdata::detect_induct_variable(flowblock *h, flowblock * &exit)
{
    list<pcodeop *>::const_reverse_iterator it;
    flowblock *true1;

    if (NULL == (exit = bblocks.detect_whiledo_exit(h))) 
        throw LowlevelError("h:%d whiledo loop not found exit node");

    true1 = h->get_true_edge()->point;

    for (it = h->ops.rbegin(); it != h->ops.rend(); it++) {
        pcodeop *p = *it;

        /* 这个地方有点硬编码了，直接扫描sub指令，这个是因为当前的测试用例中的核心VM，用了cmp指令以后
        生成了sub，这个地方可能更好的方式是匹配更复杂的pattern */
        if (p->opcode == CPUI_INT_SUB) {
            varnode *in0 = p->get_in(0);
            varnode *in1 = p->get_in(1);

            /* 假如前面是cmp指令，而且节点节点等于真值出口，那么我们认为in1是归纳变量 */
            return  (exit == true1) ? in1:in0;
        }
    }

    throw LowlevelError("loop header need CPUI_INT_SUB");
}

bool        funcdata::can_analysis(flowblock *b)
{
    return true;
}

void            varnode::add_use(pcodeop *op)
{
    uses.push_back(op);
}

void            varnode::del_use(pcodeop *op)
{
    list<pcodeop *>::iterator iter;

    iter = uses.begin();
    while (*iter != op)
        iter ++;

    uses.erase(iter);
    flags.covertdirty = 1;
}

pcodeop*        varnode::lone_use()
{
    return (uses.size() == 1) ? *(uses.begin()) : NULL;
}

bool            varnode::in_liverange(pcodeop *p)
{
	return in_liverange_simple(p);
}

bool			varnode::in_liverange_simple(pcodeop *p)
{
	int v = p->parent->fd->reset_version;

	if (v != simple_cover.version) return false;
	if (p->parent->index != simple_cover.blk_index) return false;

	int o = p->start.getOrder();
	if ((o >= simple_cover.start) && (simple_cover.start > -1)) {
		if ((o <= simple_cover.end) || (simple_cover.end == -1))
			return true;
	}

	return false;
}

bool            varnode::in_liverange(pcodeop *start, pcodeop *end)
{
    if (start->parent != end->parent)
        return false;

    list<pcodeop *>::iterator it = start->basiciter;

    for (; it != end->basiciter; it++) {
        pcodeop *p = *it;

        if (p->output && p->output->get_addr() == get_addr())
            return false;
    }

    return true;
}

void			varnode::add_def_point_simple()
{
	simple_cover.version = def->parent->fd->reset_version;
	simple_cover.blk_index = def->parent->index;
	simple_cover.start = def->start.getOrder() + 1;
	simple_cover.end = -1;
}

void			varnode::add_ref_point_simple(pcodeop *p)
{
	short v = p->parent->fd->reset_version;

	if (simple_cover.version != v) {
		if (is_input()) {
			simple_cover.blk_index = 0;
			simple_cover.version = v;
			simple_cover.start = 0;
			simple_cover.end = p->start.getOrder();
		}
		else
			throw LowlevelError("un-input varnode version mismatch");
	}
	else {
		/* 假如不是同一个块，则返回 */
		if (p->parent->index != simple_cover.blk_index)
			return;

		/* 假如是同一个块，则设置为终点 */
		simple_cover.end = p->start.getOrder();
	}
}

void			varnode::clear_cover_simple()
{
	simple_cover.version = -1;
	simple_cover.blk_index = -1;
	simple_cover.start = -1;
	simple_cover.end = -1;
}

pcodeop*        varnode::search_copy_chain(OpCode until)
{
    pcodeop *p = def;
    varnode *vn = NULL;

    while (p && (p->opcode != until)) {
        vn = NULL;
        switch (p->opcode) {
        case CPUI_STORE:
        case CPUI_LOAD:
            vn = p->get_virtualnode();
            break;

        case CPUI_COPY:
            vn = p->get_in(0);
            break;
        }

        p = (vn && vn->def) ? vn->def : NULL;
    }

    return (p && (p->opcode == until)) ? p : NULL;
}

bool            varnode::maystore_from_this(pcodeop *p)
{
    varnode *in0;

    while (p) {
        in0 = NULL;
        switch (p->opcode) {
        case CPUI_STORE:
            in0 = p->get_in(1);
            break;

        case CPUI_INT_ADD:
            if (p->get_in(1)->is_constant() && p->get_in(1)->get_val() == 0) 
                in0 = p->get_in(0);
            break;
        }

        if (in0 && in0 == this)
            return true;

        p = (in0 && in0->def) ? in0->def : NULL;
    }

    return false;
}

intb            varnode::get_val(void) const
{
    return type.v;
}

inline bool varnode_cmp_loc_def::operator()(const varnode *a, const varnode *b) const
{
    uint4 f1, f2;

    if (a->get_addr() != b->get_addr()) return (a->get_addr() < b->get_addr());
    if (a->size != b->size) return (a->size < b->size);

    f1 = ((int)a->flags.input << 1) + a->flags.written;
    f2 = ((int)b->flags.input << 1) + b->flags.written;

    /* 这样处理过后，假如一个节点是free的(没有written和Input标记)，那么在和一个带标记的
    节点比较时，就会变大，因为 0 - 1 = 0xffffffff, f1是Unsigned类型，导致他被排后 */
    if (f1 != f2) return (f1 - 1) < (f2 - 1);

    if (a->flags.written) {
        if (a->def->start != b->def->start)
            return a->def->start < b->def->start;
    }
    else if (f1 == 0)
        return a->create_index < b->create_index;

    return false;
}

inline bool varnode_cmp_def_loc::operator()(const varnode *a, const varnode *b) const
{
    uint4 f1, f2;

    f1 = ((int)a->flags.input << 1) + a->flags.written;
    f2 = ((int)b->flags.input << 1) + b->flags.written;

    if (f1 != f2) return (f1 - 1) < (f2 - 1);

    if (a->flags.written) {
        if (a->def->start != b->def->start)
            return a->def->start < b->def->start;
    }
    if (a->get_addr() != b->get_addr()) return (a->get_addr() < b->get_addr());
    if (a->size != b->size) return (a->size < b->size);

    if (f1 == 0)
        return a->create_index < b->create_index;

    return false;
}

bool pcodeop_cmp_def::operator()(const pcodeop *a, const pcodeop *b) const
{
    int c1 = a->output ? a->output->create_index : 0;
    int c2 = b->output ? b->output->create_index : 0;

    return c1 < c2;
    //return a->output->get_addr() < b->output->get_addr();
}

bool pcodeop_cmp::operator()(const pcodeop *a, const pcodeop *b) const
{
    return a->start.getTime() < b->start.getTime();
    //return a->output->get_addr() < b->output->get_addr();
}

bool pcodeop_domdepth_cmp::operator()(const pcodeop *a, const pcodeop *b) const
{
	funcdata *fd = a->parent->fd;

	return fd->domdepth[a->parent->index] < fd->domdepth[b->parent->index];
}

inline bool varnode_const_cmp::operator()(const varnode *a, const varnode *b) const
{
    return a->get_val() < b->get_val();
}

void coverblock::set_begin(pcodeop *op) 
{
	start = op->start.getOrder();
}

void coverblock::set_end(pcodeop *op) 
{
	end = op->start.getOrder();
}

void coverblock::set_end(int e)
{
	end = e;
}

bool coverblock::contain(pcodeop *op)
{
	uintm p = op->start.getOrder();

	if (empty())
		return false;

	return (p <= end) && (p >= start);
}

int coverblock::dump(char *buf)
{
	if (end == -1)
		return sprintf(buf, "{ver:%d,ind:%d,%d-max}", version, blk_index, start);
	else
		return sprintf(buf, "{ver:%d,ind:%d,%d-%d}", version, blk_index, start, end);
}

void cover::add_def_point(varnode *vn)
{
	pcodeop *def;

	c.clear();

	def = vn->get_def();
	if (def) {
		coverblock &block(c[def->parent->index]);
		block.set_begin(def);
		block.set_end(def);
	}
	else {
		throw LowlevelError("not support input varnode");
	}
}

void cover::add_ref_point(pcodeop *ref, varnode *vn, int exclude)
{
	int i;
	flowblock *bl;

	bl = ref->parent;
	coverblock &block(c[bl->index]);
	if (block.empty()) {
		uintm last = ref->start.getOrder();
		if (exclude) {
			if ((last - 1) >= 0)
				block.set_end(last - 1);
		}
		else
			block.set_end(ref);
	}
	else if (block.contain(ref)){
		return;
	}
	else {
		block.set_end(ref);
		if (block.end >= block.start) {
			return;
		}
	}

	for (i = 0; i < bl->in.size(); i++)
		add_ref_recurse(bl->get_in(i));
}

void cover::add_ref_recurse(flowblock *bl)
{
	int i;

	coverblock &block(c[bl->index]);
	if (block.empty()) {
		block.set_all();
		for (i = 0; i < bl->in.size(); i++)
			add_ref_recurse(bl->get_in(i));
	}
	else {
		if (block.end >= block.start)
			block.set_end(INT_MAX);
	}
}

bool cover::contain(const pcodeop *op)
{
    return false;
}

int cover::dump(char *buf)
{
	int n = 0;
	map<int, coverblock>::iterator it;

	n += sprintf(buf + n, "[");
	for (it = c.begin(); it != c.end(); it++) {
		coverblock &c(it->second);
		if (c.end == INT_MAX)
			n += sprintf(buf + n, "{blk:%d, %d-max}", it->first, c.start);
		else
			n += sprintf(buf + n, "{blk:%d, %d-%d}", it->first, c.start, c.end);
	}
	n += sprintf(buf + n, "]");

	return n;
}

pcodeop::pcodeop(int s, const SeqNum &sq)
    :start(sq), inrefs(s)
{
    memset(&flags, 0, sizeof(flags));
    flags.dead = 1;
    parent = 0;

    output = 0;
    opcode = CPUI_MAX;
}
pcodeop::~pcodeop()
{
}

void    pcodeop::set_opcode(OpCode op)
{
    if (opcode != CPUI_MAX)
        flags.changed = 1;

    opcode = op;

    /* FIXME:后面要切换成typefactory，参考Ghidra */
    if (opcode == CPUI_CBRANCH || opcode == CPUI_BRANCH || opcode == CPUI_CALL)
        flags.coderef = 1;
}

void            pcodeop::clear_input(int slot) 
{
    inrefs[slot] = NULL; 
}

void    pcodeop::remove_input(int slot)
{
    for (int i = slot + 1; i < inrefs.size(); i++)
        inrefs[i - 1] = inrefs[i];

    inrefs.pop_back();
}

void    pcodeop::insert_input(int slot)
{
    int i;
    inrefs.push_back(NULL);
    for (i = inrefs.size() - 1; i > slot; i--)
        inrefs[i] = inrefs[i - 1];
    inrefs[slot] = NULL;
}

// 扩展outbuf的内容区，用来做对齐用 
#define expand_line(num)        while (i < num) buf[i++] = ' '
int             pcodeop::dump(char *buf, uint32_t flags)
{
    int i = 0, j, in_limit = 10000;
    dobc *d = parent->fd->d;
    Translate *trans = parent->fd->d->trans;

    i += sprintf(buf + i, " p%-3d [%3d]:", start.getTime(), start.getOrder());

    if (output) {
        i += print_varnode(trans, buf + i, output);
        i += sprintf(buf + i, " = ");
    }

    i += sprintf(buf + i, "%s", get_opname(opcode));
    // Possibly check for a code reference or a space reference

    if (flags & PCODE_OMIT_MORE_IN) in_limit = 4;

    if (callfd) 
        i += sprintf(buf + i, ".%s ", callfd->name.c_str());

    for (j = 0; j < inrefs.size(); ++j) {
        if (j == in_limit) break;
        i += sprintf(buf + i, " ");
        i += print_varnode(trans, buf + i, inrefs[j]);
    }

    if (j == in_limit)
        i += sprintf(buf + i, "[...]");

    expand_line(48);

    if (flags & PCODE_DUMP_VAL) {
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, "<font color=\"red\"> ");
        i += print_vartype(trans, buf + i, output);
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, " </font>");
    }

    if (flags & PCODE_DUMP_UD)
        i += print_udchain(buf + i, this, flags);

    if (is_call()) {
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, "<font color=\"red\"> ");
#if 0
        i += sprintf(buf + i, "[r0:");
        i += print_vartype(trans, buf + i, callctx->r0);

        i += sprintf(buf + i, ",r1:");
        i += print_vartype(trans, buf + i, callctx->r1);

        i += sprintf(buf + i, ",r2:");
        i += print_vartype(trans, buf + i, callctx->r2);

        i += sprintf(buf + i, ",r3:");
        i += print_vartype(trans, buf + i, callctx->r3);
#endif

        i += sprintf(buf + i, "[sp:");
        i += print_vartype(trans, buf + i, get_in(d->sp_addr));
        i += sprintf(buf + i, "]");
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, " </font>");
    }

    if (this->flags.vm_eip) {
        i += sprintf(buf + i, "<font color=\"green\">**VMEIP</font>");
    }

    buf[i] = 0;

    return i;
}

bool            pcodeop::in_sp_alloc_range(varnode *pos)
{
    if (opcode != CPUI_INT_SUB)  return false;

    Address &sp = parent->fd->d->sp_addr;

    varnode *in0 = get_in(0);
    varnode *in1 = get_in(1);

    if (output && output->is_sp_constant()
        && in0->is_sp_constant()
        && in1->is_constant()
        && pos->is_sp_constant()
        && (-pos->get_val() > -in0->get_val()) 
        && (-pos->get_val() <= -output->get_val())) {
        return true;
    }

    return false;
}

void            pcodeop::on_MULTIEQUAL()
{
    varnode *vn, *cn = NULL, *vn1;
    vector<varnode *> philist;
    set<intb> c;
    pcodeop_set visit;
    pcodeop_set::iterator it;
    pcodeop *p1, *p;
    int i;

    visit.insert(this);

    for (i = 0; i < inrefs.size(); i++) {
        vn = get_in(i);
        p = vn->def;
        if (vn->is_constant()) {
            c.insert(vn->get_val());
            cn = vn;
            continue;
        }
        else if (!p) {
            output->set_top();
            return;
        }
        else if (p->opcode != CPUI_MULTIEQUAL) {
            p = vn->search_copy_chain(CPUI_MULTIEQUAL);

            if (!p) {
                output->set_top();
                return;
            }
        }

        if (p->opcode == CPUI_MULTIEQUAL) {
            it = visit.find(p);
            if (it == visit.end())
                philist.push_back(p->output);
        }
    }

    /* 假如有2个常量定值进来，那么值的height必然是T */
    if (c.size() > 1) {
        output->type.height = a_top;
        return;
    }

    /* 假如所有的常量的值，完全一样，而且philist节点为空，那么设置phi节点的out值为常量*/
    if (philist.size() == 0) {
        output->type = cn->type;
        return;
    }

    /* 
    1. 假如一个phi节点中，所有常量的值相等
    2. 另外的非常量节点都是Phi节点
    3. 递归的遍历其中的phi节点，重新按1,2开始
    4. 遍历完成后，那么此phi节点也是常量
    */
    while (!philist.empty()) {
        vn = philist.front();
        philist.erase(philist.begin());

        p = vn->def;
        it = visit.find(p);
        if (it != visit.end()) continue;

        visit.insert(p);

        for (i = 0; i < p->inrefs.size(); i++) {
            vn1 = p->get_in(i);
            p1 = vn1->def;

            if (vn1->is_constant()) {
                if (!cn) cn = vn1;
                else if (vn1->type != cn->type) {
                    output->set_top();
                    return;
                }
                continue;
            }
            else if (!p1) {
                output->set_top();
                return;
            }
            else if (p1->opcode != CPUI_MULTIEQUAL) {
                p1 = vn1->search_copy_chain(CPUI_MULTIEQUAL);

                if (!p1) {
                    output->set_top();
                    return;
                }
            }

            if (p1->opcode == CPUI_MULTIEQUAL) {
                it = visit.find(p1);
                if (it == visit.end()) {
                    philist.push_back(p1->output);
                }
            }
        }
    }

    if (cn)
        output->type = cn->type;
    else
        output->set_top();
}

void            pcodeop::loadram2out(Address &addr)
{
    dobc *d = parent->fd->d;
    unsigned char buf[8];
    varnode *out = output;
    int ret;

    memset(buf, 0, sizeof(buf));
    ret = d->loader->loadFill(buf, out->size, addr);
    if (ret == DATA_TOP) {
        out->set_top();
        return;
    }

    if (out->size == 1)
        out->set_val(*(int1 *)buf);
    else if (out->size == 2)
        out->set_val(*(int2 *)buf);
    else if (out->size == 4)
        out->set_val(*(int *)buf);
    else if (out->size == 8)
        out->set_val(*(intb *)buf);
}

bool            pcodeop::all_inrefs_is_constant(void)
{
    int i;

    for (i = 0; i < inrefs.size(); i++) {
        if (!inrefs[i]->is_constant()) return false;
    }
    return true;
}

bool            pcodeop::all_inrefs_is_adj(void)
{
    int i;

    for (i = 0; i < inrefs.size(); i++) {
        pcodeop *p = inrefs[i]->def;
        if (!p) return false;
        if (parent->is_adjacent(p->parent)) continue;
        return false;
    }
    return true;
}

int				pcodeop::compute_add_sub()
{
	varnode *in0, *in1, *_in0, *_in1, *out;
	pcodeop *op;
	funcdata *fd = parent->fd;

	in0 = get_in(0);
	in1 = get_in(1);
	out = output;

	op = in0->def;

	if (!is_trace() && !(dobc::singleton()->is_simd(get_addr())) && op && ((op->opcode == CPUI_INT_ADD) || (op->opcode == CPUI_INT_SUB))) {
		_in0 = op->get_in(0);
		_in1 = op->get_in(1);

		/*
		ma = ma + 4;
		x = ma + 4;

		转换成
		x = ma + 8;

		关于活跃范围判断有2种情况

		1. r0 = r1 + 4
		2. r1 = r4 + 4
		3. sp = r0 + 4

		假如你想优化 sp = r1 + 4，需要判断 指令3 是不是在r1的活动范围内

		1. ma = ma + 4
		2. sp = ma + 4

		当前r0, r1位置的寄存器相等时，需要判断ma的范围
		*/
        while (
            _in0->is_sp_constant()
            && !dobc::singleton()->is_simd(op->get_addr())
			&& (in0->uses.size() == 1) && _in1->is_constant() 
			&& ((op->output->get_addr() == _in0->get_addr()) || _in0->in_liverange_simple(this))) {
			intb v = in1->get_val();

			if (opcode == CPUI_INT_ADD) {
				if (op->opcode == CPUI_INT_ADD)
					v = _in1->get_val() + v;
				else
					v = -_in1->get_val() + v;
			}
			else {
				if (op->opcode == CPUI_INT_ADD)
					v = -_in1->get_val() + v;
				else
					v = _in1->get_val() + v;
			}

			while (num_input() > 0)
				fd->op_remove_input(this, 0);

			fd->op_set_input(this, in0 = _in0, 0);
			fd->op_set_input(this, in1 = fd->create_constant_vn(v, in1->size), 1);
			fd->op_destroy(op);

			op = _in0->def;
			if ((op->opcode != CPUI_INT_ADD) && (op->opcode != CPUI_INT_SUB))
				break;
			_in0 = op->get_in(0);
			_in1 = op->get_in(1);
		}
	}

	if (opcode == CPUI_INT_ADD)
		out->set_sp_constant(in0->type.v + in1->type.v);
	else
		out->set_sp_constant(in0->type.v - in1->type.v);

	return 0;
}

int             pcodeop::compute(int inslot, flowblock **branch)
{
    varnode *in0, *in1, *in2, *out, *_in0, *_in1, *vn;
    funcdata *fd = parent->fd;
    dobc *d = fd->d;
    int ret = 0, i;
    pcodeop *store, *op, *op1;
    flowblock *b, *bb;

    out = output;
    in0 = get_in(0);

    switch (opcode) {
    case CPUI_COPY:
        if (!is_trace() && !in0->is_input() && ((op = in0->def) && op->opcode == CPUI_COPY)) {
            _in0 = op->get_in(0);
            if ((_in0->get_addr() == out->get_addr()) && ((_in0->version + 1) == out->version)) {
                to_copy(_in0);

                if (in0->uses.size() == 0)
                    fd->op_destroy(op);
            }
        }

        if (in0->get_addr().getSpace() == d->ram_spc)
            loadram2out(Address(d->trans->getDefaultCodeSpace(), in0->get_addr().getOffset()));
        else if (in0->is_constant()) {
            if ((in0->size == 16) && in0->get_val()) {
                //vm_error("pcode in size == 16, pcode(p%d)\n", start.getTime());
            }

            out->set_val(in0->get_val());
        }
        else if (fd->is_sp_constant(in0)) {
            out->set_sp_constant(in0->get_val());

            /* 识别这种特殊形式

            sp = ma + 4;

            ma = sp

            转换成
            ma = ma + 4;

            不处理load是怕会影响别名分析
            */
            op = in0->def;
            if (!is_trace()
                && (in0->uses.size() == 1)
                && !in0->flags.input
                && (op->opcode == CPUI_INT_ADD)) {
                _in0 = op->get_in(0);
                _in1 = op->get_in(1);

                /* 后面那个判断都是用来确认，活跃范围的 */
                //if (_in1->is_constant() && (_in0->get_addr() == output->get_addr()) && ((_in0->version + 1) == (output->version))) {
                if (_in1->is_constant() && _in0->in_liverange(this)) {
                    fd->op_remove_input(this, 0);
                    fd->op_set_opcode(this, op->opcode);

                    for (int i = 0; i < op->num_input(); i++) {
                        fd->op_set_input(this, op->get_in(i), i);
                    }

                    fd->op_destroy(op);
                }
            }
        }
        else
            out->type = in0->type;

        break;

        //      out                            in0              in1            
        // 66:(register,r1,4) = LOAD (const,0x11ed0f68,8) (const,0x840c,4)
    case CPUI_LOAD:
        fd->vmp360_marker(this);

        in1 = get_in(1);
        in2 = (inrefs.size() == 3) ? get_in(2):NULL;
        if (fd->is_code(in0, in1) && (in1->is_constant() || in1->is_pc_constant())) {
            loadram2out(Address(d->trans->getDefaultCodeSpace(), in1->type.v));
            //printf("addr=%llx, pcode=%d, load ram, pos = %llx, val = %llx\n", get_dis_addr().getOffset(), start.getTime(), in1->type.v, out->get_val());
        }
        else if (in2) { // 别名分析过
            output->type = in2->type;
            op = in2->def;
            varnode *_in2 = op->get_in(2);

            /* 
            mem[x] = r0(0)

            r0(1) = mem[x]
            修改第2条指令会 cpy r0(1), r0(0)
            */
#if 1
            if (!is_trace() && 
				(((_in2->get_addr() == out->get_addr()) && (_in2->version + 1) == (out->version))
					|| _in2->in_liverange_simple(this))) {
                while (num_input())
                    fd->op_remove_input(this, 0);

                fd->op_set_opcode(this, CPUI_COPY);
                fd->op_set_input(this, _in2, 0);
            }
#else
            if (!is_trace() && _in2->in_liverange_simple(this)) {
                while (num_input())
                    fd->op_remove_input(this, 0);

                fd->op_set_opcode(this, CPUI_COPY);
                fd->op_set_input(this, _in2, 0);
            }
#endif
        }
        else if ((inslot >= 0)) { // trace流中
            pcodeop *maystoer = NULL;
            if ((store = fd->trace_store_query(this))) {
                if (store->opcode == CPUI_INT_SUB)
                    out->set_val(0);
                else
                    out->type = store->get_in(2)->type;
            }
            else {
                /* 这里有点问题，实际上可能是a_bottom */
                out->type.height = a_top;
            }
        }
        /* 假如这个值确认来自于外部，不要跟新他 */
        else if (!flags.input && !flags.val_from_sp_alloc)
            out->type.height = a_top;

        if (out->is_constant()) {
        }
        else if (in2) {
            varnode* _in2 = in2->def->get_in(2);

            /*
            mem[x] = a1;
            a2 = mem[x];

            转换成
            a2 = a1
            */
#if 0
            if ((output->get_addr() == _in2->get_addr()) && (output->version == (_in2->version + 1))) {
                while (num_input() > 0)
                    fd->op_remove_input(this, 0);

                fd->op_set_opcode(this, CPUI_COPY);
                fd->op_set_input(this, _in2, 0);
            }
#endif
        }

        break;

        //
    case CPUI_STORE:
        fd->vmp360_marker(this);

        in1 = get_in(1);
        in2 = get_in(2);
        if (output) {
            output->type = in2->type;
        }

        /*
        a = mem[x];
        mem[x] = a;
        消减掉这种形式的代码
        */
#if 0
        if ((op = in2->def) && (op->opcode == CPUI_LOAD) && (in2->uses.size() == 1) && (in1->type == op->get_in(1)->type)) {
            fd->op_destroy(op);
            fd->op_destroy(this);
            return ERR_FREE_SELF;
        }
#endif

        break;

    case CPUI_BRANCH:
        in0 = get_in(0);

        if (!flags.branch_call) {
            *branch = parent->get_out(0);
        }
        ret = ERR_MEET_CALC_BRANCH;
        break;

    case CPUI_CBRANCH:
        in1 = get_in(1);
        /* 
        两种情况
        1. 
        2. */
        if ((in1->is_constant())) {
            blockedge *edge; 
            
            edge = in1->get_val() ? parent->get_true_edge() : parent->get_false_edge();
            *branch = edge->point;
            ret = ERR_MEET_CALC_BRANCH;

            if (!is_trace() && !fd->in_cbrlist(this))
                fd->cbrlist.push_back(this);
        }
        else if ((op = in1->def) && (op->opcode == CPUI_BOOL_NEGATE)
            && (_in0 = op->get_in(0)) && _in0->def && (_in0->def->opcode == CPUI_BOOL_NEGATE)
            && (vn = _in0->def->get_in(0))->in_liverange(_in0->def, this)) {
            fd->op_remove_input(this, 1);
            fd->op_set_input(this, vn, 1);
        }
        break;

    case CPUI_BRANCHIND:
        if (in0->is_constant()) {
            Address addr(d->get_code_space(), in0->get_val());

            if (!(op = fd->find_op(addr))) {
                printf("we found a new address[%llx] to analaysis\n", addr.getOffset());
                fd->addrlist.push_back(addr);
            }
            else {
                *branch = op->parent;
                ret = ERR_MEET_CALC_BRANCH;
            }
        }
        break;

    case CPUI_INT_EQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() == in1->get_val());
        }
        /*
        
        a = phi(c2, c3)
        b = c1
        d = a - b
        e = INT_EQUAL d, 0
        假如c1 不等于 c2, c3
        那么 e 也是可以计算的。

        NOTE:这样写的pattern会不会太死了？改成递归收集a的所有常数定义，但是这样会不会效率太低了。
        */
        else if (in1->is_constant()  && (in1->get_val() == 0)
            && (op = in0->def) && (op->opcode == CPUI_INT_SUB)
            && (op->get_in(0)->is_constant() || op->get_in(1)->is_constant())) {
            _in0 = op->get_in(0);
            _in1 = op->get_in(1);

            varnode *check = _in0->is_constant() ? _in1 : _in0;
            varnode *uncheck = _in0->is_constant() ? _in0 : _in1;
            pcodeop *phi = check->def;
            int equal = 0, notequal = 0;  // 2:top, 1:equal, 0:notequal

            output->set_top();
            if (phi && (phi->opcode == CPUI_MULTIEQUAL)) {
                for (i = 0; i < phi->inrefs.size(); i++) {
                    vn = phi->get_in(i);
                    if (!vn->is_constant()) break;

                    if (vn->get_val() != uncheck->get_val()) notequal++;
                    else equal++;
                }

                if (notequal == phi->inrefs.size())     
                    output->set_val(0);
                else if (equal == phi->inrefs.size())   
                    output->set_val(1);
            }
        }
        else
            output->set_top();
        break;

    case CPUI_INT_NOTEQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() != in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_SLESS:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() < in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_SLESSEQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() <= in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_LESS:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val((uint8)in0->get_val() < (uint8)in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_LESSEQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val((uint8)in0->get_val() <= (uint8)in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_ZEXT:
        if (in0->is_constant()) {
            if (in0->size < output->size) {
                output->set_val(zext(in0->get_val(), in0->size, out->size));
            }
            else if (in0->size > output->size) {
                throw LowlevelError("zext in size > ouput size");
            }
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_SEXT:
        if (in0->is_constant()) {
            if (in0->size < output->size) {
                output->set_val(sext(in0->get_val(), in0->size, out->size));
            }
            else if (in0->size > output->size) {
                throw LowlevelError("zext in size > ouput size");
            }
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_ADD:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            if (in0->size == 1)
                out->set_val((int1)in0->type.v + (int1)in1->type.v);
            else if (in0->size == 2)
                out->set_val((int2)in0->type.v + (int2)in1->type.v);
            else if (in0->size == 4)
                out->set_val((int4)in0->type.v + (int4)in1->type.v);
            else 
                out->set_val(in0->type.v + in1->type.v);
        }
        else if ((in0->is_pc_constant() && in1->is_constant())
            || (in0->is_constant() && in1->is_pc_constant())) {
            out->set_pc_constant(in0->get_val() + in1->get_val());
        }
        else if (fd->is_sp_constant(in0) && in1->is_constant()) {
			compute_add_sub();
        }
        else if (in0->is_constant() && fd->is_sp_constant(in1)) {
            out->set_sp_constant(in0->type.v + in1->type.v);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_SUB:
        in1 = get_in(1);
        op = in0->def;

        if (in0->is_constant() && in1->is_constant()) {
            if (in0->size == 1)
                out->set_val((int1)in0->type.v - (int1)in1->type.v);
            else if (in0->size == 2)
                out->set_val((int2)in0->type.v - (int2)in1->type.v);
            else if (in0->size == 4)
                out->set_val((int4)in0->type.v - (int4)in1->type.v);
            else 
                out->set_val(in0->type.v - in1->type.v);
        }
        /*      out                             0                   1       */
        /* 0:(register,mult_addr,4) = INT_SUB (register,sp,4) (const,0x4,4) */
        else if (fd->is_sp_constant(in0) && in1->is_constant()) {
			compute_add_sub();
        }
        else if (in0->is_pc_constant() && in1->is_constant()) {
            out->set_pc_constant(in0->get_val() - in1->get_val());
        }
        /*
        peephole:

        a = c1;
        if (x) a = c2;

        if (a == c1) {
        }
        else {
            if (a == c2) {
                // block1
            }
            else {
                // block 2
            }
        }
        实际上block2是死的，因为a只有2个定值
        */
        else if (in0->is_top() && in1->is_constant() 
            && (parent->in.size() == 1)
            && (op = in0->def) && (op->opcode == CPUI_MULTIEQUAL) && (op->inrefs.size() == 2) && op->all_inrefs_is_constant() 
            && (op1 = (b = parent->get_in(0))->get_cbranch_sub_from_cmp())
            && op1->get_in(1)->is_constant()
            && (op1->get_in(0) == in0)
            && b->is_eq_cbranch()) {
            intb imm = (op->get_in(0)->get_val() == op1->get_in(1)->get_val()) ? op->get_in(1)->get_val() : op->get_in(0)->get_val();
            out->set_sub_val(in0->size, imm, in1->get_val());
        }
        else {
            out->type.height = a_top;
        }
        break;

    case CPUI_INT_SBORROW:
        in1 = get_in(1);
        /* 
        wiki: The overflow flag is thus set when the most significant bit (here considered the sign bit) is changed 
        by adding two numbers with the same sign (or subtracting two numbers with opposite signs).

        SBORROW一般是用来设置ov标记的，根据wiki上的说法，当加了2个一样的数，MSB发生了变化时，设置overflow flag
        */
        if (in0->is_constant() && in1->is_constant()) {
            int e = 1;

            if (in0->size == 4) {
                int l = (int)in0->get_val();
                int r = -(int)in1->get_val();
                int o;

                if (MSB4(l) != MSB4(r)) {
                    out->set_val(1);
                }
                else {
                    o = l + r;
                    if (MSB4(o) != MSB4(l)) {
                        e = 0;
                        out->set_val(1);
                    }
                }
            }
            else {
                throw LowlevelError("not support");
            }

            if (e) out->set_val(0);
        }
        break;

    case CPUI_INT_2COMP:
        if (in0->is_constant()) {
            out->set_val1(-in0->get_val());
        }
        else
            out->set_top();
        break;

    case CPUI_INT_LEFT:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val1((uintb)in0->get_val() << (uintb)in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_RIGHT:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val1((uintb)in0->get_val() >> (uintb)in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_NEGATE:
        if (in0->is_constant()) {
            intb v = in0->get_val();
            out->set_val(~v);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_XOR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() ^ in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_AND:
        in1 = get_in(1);
        uintb l, r;
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() & in1->get_val());
        }
        /*
        识别以下pattern:

        t1 = r6 * r6
        r2 = t1 + r6 (r2一定为偶数)

        r2 = r2 & 1 一定为0
        */
        else if (in1->is_constant() 
            && (in1->get_val() == 1) 
            && in0->def 
            && (in0->def->opcode == CPUI_INT_ADD)
            && (op = in0->def->get_in(0)->def)
            && (op->opcode == CPUI_INT_MULT)
            && (op->get_in(0)->get_addr() == op->get_in(1)->get_addr())
            && (op->get_in(0)->get_addr() == in0->def->get_in(1)->get_addr())) {
            out->set_val(0);
        }
        /* (sp - even) & 0xfffffffe == sp - even
        因为sp一定是偶数，减一个偶数，也一定还是偶数，所以他和 0xfffffffe 相与值不变
        */
        else if (in0->is_sp_constant() && in1->is_constant() 
            && ((l = (in0->get_val() & in1->get_val())) == (r = (in0->get_val() & (((uintb)1 << (in0->size * 8)) - 1))))) {
            out->set_sp_constant(in0->get_val() & in1->get_val());
        }
        else if (in0->is_pc_constant() && in1->is_constant()) {
            out->set_pc_constant(in0->get_val() & in1->get_val());
        }
        /* 相与时，任意一个数为0，则为0 */
        else if ((in0->is_constant() && (in0->get_val() == 0)) || (in1->is_constant() && (in1->get_val() == 0))) {
            out->set_val(0);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_OR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() | in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_MULT:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() * in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_NEGATE:
        if (in0->is_constant()) {
            out->set_val(in0->get_val() ? 0:1);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_XOR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() ^ in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_AND:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() & in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_OR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() | in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_MULTIEQUAL:
        if (inslot >= 0) {
            output->type = get_in(inslot)->type;
        }
        else if (!flags.force_constant){
            /* 
            int x, y;
            if (!a) {
                y = 0;
            }

            if (a) {
                y = 0;
            }

            */
            if ((inrefs.size() == 2) && (in1 = get_in(1))->is_constant() 
                && in0->def && in0->def->opcode == CPUI_MULTIEQUAL
                && (_in1 = in0->def->get_in(1))
                && _in1->is_constant() && (in1->get_val() == _in1->get_val())
                && in1->def
                && (in1->def->parent->in.size() == 1) 
                && (b = in1->def->parent->get_in(0))
                && b->ops.size()
                && (b->last_op()->opcode == CPUI_CBRANCH)
                && (_in1->def->parent->in.size() == 1)
                && (bb = _in1->def->parent->get_in(0))
                && (bb->last_op()->opcode == CPUI_CBRANCH)
                && (vn = bb->last_op()->get_in(1))
                && vn->def
                && (vn->def->opcode == CPUI_BOOL_NEGATE)
                && (vn = vn->def->get_in(0))
                && (b->last_op()->get_in(1) == vn)) {
                output->set_val(in1->get_val());
            }
            else {
#if 0
                for (i = 1; i < inrefs.size(); i++) {
                    in1 = get_in(i);
                    if (in0->type != in1->type)
                        break;
                }

                if (i == inrefs.size())
                    output->type = in0->type;
                else
                    output->type.height = a_top;
#else
                on_MULTIEQUAL();
#endif
            }
        }
        break;

    case CPUI_SUBPIECE:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            int v = in1->get_val();
            out->set_val((in0->get_val() >> (v * 8)) & calc_mask(out->size));
        }
        break;

    default:
        break;
    }

    /* 返回跳转地址 */
    if ((this == parent->last_op()) && (parent->out.size() == 1))
        *branch = parent->get_out(0);

    return ret;
}

void            pcodeop::to_constant(void)
{
    funcdata *fd = parent->fd;

    while (num_input() > 0)
        fd->op_remove_input(this, 0);

    fd->op_set_opcode(this, CPUI_COPY);
    fd->op_set_input(this, fd->create_constant_vn(output->get_val(), output->size), 0);
}

void            pcodeop::to_constant1(void)
{
    funcdata *fd = parent->fd;
    int i;
    varnode *vn, *out = output;
    pcodeop *op;

    /*
    非trace条件才能开启常量持久化
    */
    if (is_trace()) return;

    /* simd别转了 */
    if (dobc::singleton()->is_simd(get_addr()) && fd->flags.disable_simd_to_const) return;
    /* FIXME:
    这里不对simd的phi节点做转换是为了在代码生成时减少工作量，否则我们需要在某些cfg头部插入vmov_imm的指令
    
    是否考虑修复掉他？  */
    if ((opcode == CPUI_MULTIEQUAL) && fd->d->is_vreg(poa(this))) return;

    /* 
    1. 当有output节点，且output节点为常量时，运行进行常量持久化 
    2. opcode不能为copy，因为常量持久化就是把常量节点，改成copy操作 
    3. output节点为temp的不允许进行常量化，这个会破坏原始的inst结构，导致无法识别出原先的inst，在codegen时加大了难度
    4. opcode不能为store，因为store有副作用，它的use也不是特别好计算 
    */
    if (output && output->is_constant() 
        && !dobc::singleton()->is_temp(output->get_addr()) 
        && (opcode != CPUI_COPY) 
        && (opcode != CPUI_STORE)) {
        /* phi节点转成 copy指令时，需要记录下这个copy节点来自于phi节点，在删除phi节点时，假如有需要可以删除这个copy  */
        if (opcode == CPUI_MULTIEQUAL) flags.copy_from_phi = 1;
        while (num_input() > 0) 
            fd->op_remove_input(this, 0);

        fd->op_set_opcode(this, CPUI_COPY);
        fd->op_set_input(this, fd->create_constant_vn(out->get_val(), out->size), 0);
    }
    /* 
    1. phi节点的in节点不能这么处理
    2. call节点不能常量持久化in， 
    3. store节点的in节点不能转换为常量，这个是因为thumb不支持 store 一个 imm 到一个地址上，假如我们常量化会导致不好做代码生成

    */
    else if (!fd->flags.disable_inrefs_to_const && (opcode != CPUI_MULTIEQUAL) && (opcode != CPUI_STORE) && !is_call()) {
        for (i = 0; i < inrefs.size(); i++) {
            vn = get_in(i);

            /* 后面那个判断条件是防止冲入的*/
            if (vn->is_constant()) {
                if (!vn->in_constant_space()) {
                    fd->op_unset_input(this, i);
                    fd->op_set_input(this, fd->create_constant_vn(vn->get_val(), vn->size), i);
                }
            }
            /* 这里重点说下in的非constant转换规则 

            output = op(in0, in1, ..., inN) 
            上面的指令中，
            1. 假如某个inN的def是一个copy的操作
            2. 拷贝来自于另外一个cpu寄存器(rN)
            3. 当前的pcode在那个rN的活跃范围内
            我们直接修改上面的指令为 
            output = op(in0, in1, ..., rN)
            */
            else if ((op = vn->def) && (op->opcode == CPUI_COPY) && op->get_in(0)->in_liverange_simple(this)) {
                fd->op_unset_input(this, i);
                fd->op_set_input(this, op->get_in(0), i);
            }
        }
    }
}

void            pcodeop::to_rel_constant()
{
    funcdata *fd = parent->fd;
    if (opcode != CPUI_MULTIEQUAL)
        throw LowlevelError("to_rel_constant() only support MULTIEQUAL");

    varnode *in0 = get_in(0);

    while (num_input() > 1)
        fd->op_remove_input(this, 1);

    fd->op_set_opcode(this, CPUI_INT_ADD);
    intb sub = output->get_val() - in0->get_val();
    fd->op_set_input(this, fd->create_constant_vn(sub, output->size), 1);
}

void            pcodeop::to_copy(varnode *in)
{
    funcdata *fd = parent->fd;

    while (num_input() > 0)
        fd->op_remove_input(this, 0);

    fd->op_set_opcode(this, CPUI_COPY);
    fd->op_set_input(this, in, 0);
}

void            pcodeop::to_nop(void)
{
    funcdata *fd = parent->fd;

    while (num_input() > 0)
        fd->op_remove_input(this, 0);

    if (output)
        fd->destroy_varnode(output);

    fd->op_set_opcode(this, CPUI_COPY);
}

flowblock::flowblock(funcdata *f)
{
    fd = f;
}

flowblock::~flowblock()
{
}

op_edge::op_edge(pcodeop *f, pcodeop *t)
{
    from = f;
    to = t;
}

op_edge::~op_edge()
{
}

jmptable::jmptable(pcodeop *o)
{
    op = o;
    opaddr = o->get_addr();
}

jmptable::jmptable(const jmptable *op2)
{
    op = op2->op;
    opaddr = op2->opaddr;
    defaultblock = op2->defaultblock;
    lastblock = op2->lastblock;
    size = op2->size;
    addresstable = op2->addresstable;
}

jmptable::~jmptable()
{
}

void    jmptable::update(funcdata *fd)
{
    op = fd->find_op(opaddr);
}

rangenode::rangenode()
{
}

rangenode::~rangenode()
{
}

ollvmhead::ollvmhead(flowblock *h1)
{
    h = h1;
}

ollvmhead::ollvmhead(Address &a, flowblock *h1)
    :st1(a)
{
    h = h1;
}

ollvmhead::ollvmhead(Address &a, Address &b, flowblock *h1)
    : st1(a), st2(b)
{
    h = h1;
}

ollvmhead::~ollvmhead()
{
}

void blockgraph::add_block(blockbasic *b)
{
    int min = b->index;

    if (blist.empty())
        index = min;
    else {
        if (min < index) index = min;
    }

    b->parent = this;
    blist.push_back(b);
}

void blockgraph::find_spanning_tree(vector<flowblock *> &preorder, vector<flowblock *> &rootlist)
{
    if (blist.size() == 0) return;

    int i, origrootpos;
    vector<flowblock *> rpostorder;
    vector<flowblock *> state;
    int *visitcount;
    int rpostcount = blist.size();
    flowblock *tmpbl, *child;

    preorder.reserve(blist.size());
    rpostorder.resize(blist.size());
    visitcount = (int *)calloc(1, sizeof(int) * blist.size());

    exitlist.clear();
    clear_loopinfo();
    for (i = 0; i < blist.size(); i++) {
        tmpbl = blist[i];
        tmpbl->index = -1;
        tmpbl->dfnum = -1;
        tmpbl->copymap = tmpbl;
        if ((tmpbl->in.size() == 0))
            rootlist.push_back(tmpbl);

        if (!tmpbl->out.size())
            exitlist.push_back(tmpbl);

        tmpbl->clear_loopinfo();
    }
    if (rootlist.size() != 1) {
        throw LowlevelError("more root head");
    }

    origrootpos = rootlist.size() - 1;

    visitcount[0] = 0;
    state.push_back(blist[0]);
    preorder.push_back(blist[0]);
    blist[0]->dfnum = 0;

    while (!state.empty()) {
        flowblock *bl = state.back();

        int index = visitcount[bl->dfnum];

        /* 当前节点的子节点都遍历完成 */
        if (index == bl->out.size()) {
            state.pop_back();
            bl->index = --rpostcount;
            rpostorder[rpostcount] = bl;
            if (!state.empty())
                state.back()->numdesc += bl->numdesc;
        }
        else {
            blockedge &e = bl->out[index];
            child = e.point;
            visitcount[bl->dfnum] += 1;

            /* */
            if (child->dfnum == -1) {
                bl->set_out_edge_flag(index, a_tree_edge);
                state.push_back(child);

                child->dfnum = preorder.size();
                /* dfs顺序的就是先序遍历 */
                preorder.push_back(child);
                visitcount[child->dfnum] = 0;
                child->numdesc = 1;
            }
            /* 假如发现out边上的节点指向的节点，是已经被访问过的，那么这是一条回边 */
            else if (child->index == -1) {
                bl->set_out_edge_flag(index, a_back_edge | a_loop_edge);
            }
            /**/
            else if (bl->dfnum < child->dfnum) {
                bl->set_out_edge_flag(index, a_forward_edge);
            }
            else
                bl->set_out_edge_flag(index, a_cross_edge);
        }
    }

    free(visitcount);
    blist = rpostorder;
}

/*
1. 找到不可规约边
2. 找到 spanning tree(计算df需要)
3. 设置flowblock的索引为反向支配顺序
4. 标记 tree-edge, forward-edges, cross-edges, 和 back-edge
    初步怀疑: tree-edge 是spanning tree
              forward-edge 是
*/
void blockgraph::structure_loops(vector<flowblock *> &rootlist)
{
    vector<flowblock *> preorder;
    int irreduciblecount = 0;

    find_spanning_tree(preorder, rootlist);
    /* vm360的图是不可规约的，还不确认不可规约的图会对优化造成什么影响 */
    find_irreducible(preorder, irreduciblecount);
}

void blockgraph::dump_spanning_tree(const char *filename, vector<flowblock *> &rootlist)
{
    FILE *fp;
    int i;

    fp = fopen(filename, "w");

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    for (i = 0; i < rootlist.size(); i++) {
    }

    fclose(fp);
}

/*

paper: A Simple, Fast Dominance Algorithm
http://web.cse.ohio-state.edu/~rountev.1/788/papers/cooper-spe01.pdf
*/
void  blockgraph::calc_forward_dominator(const vector<flowblock *> &rootlist)
{
    vector<flowblock *>     postorder;
    flowblock *b, *new_idom, *rho;
    bool changed;
    int i, j, finger1, finger2;

    if (blist.empty())
        return;

    if (rootlist.size() > 1)
        throw LowlevelError("we are not support rootlist.size() exceed 1");

    int numnodes = blist.size() - 1;
    postorder.resize(blist.size());
    for (i = 0; i < blist.size(); i++) {
        blist[i]->immed_dom = NULL;
        postorder[numnodes - i] = blist[i];
    }

    b = postorder.back();
    if (b->in.size()) {
        throw LowlevelError("entry node in edge error");
    }

    b->immed_dom = b;
    for (i = 0; i < b->out.size(); i++)
        b->get_out(i)->immed_dom = b;

    changed = true;
    new_idom = NULL;

    while (changed) {
        changed = false;
        for (i = postorder.size() - 2; i >= 0; --i) {
            b = postorder[i];

            /* 感觉这个判断条件是不需要的，但是Ghdira源代码里有 */
            if (b->immed_dom == postorder.back()) {
                continue;
            }

            for (j = 0; j < b->in.size(); j++) {
                new_idom = b->get_in(j);
                if (new_idom->immed_dom)
                    break;
            }

            j += 1;
            for (; j < b->in.size(); j++) {
                rho = b->get_in(j);
                if (rho->immed_dom) {
                    finger1 = numnodes - rho->index;
                    finger2 = numnodes - new_idom->index;
                    while (finger1 != finger2) {
                        while (finger1 < finger2)
                            finger1 = numnodes - postorder[finger1]->immed_dom->index;
                        while (finger2 < finger1)
                            finger2 = numnodes - postorder[finger2]->immed_dom->index;
                    }
                    new_idom = postorder[finger1];
                }
            }
            if (b->immed_dom != new_idom) {
                b->immed_dom = new_idom;
                changed = true;
            }
        }
    }

    postorder.back()->immed_dom = NULL;
}

blockbasic* blockgraph::new_block_basic(void)
{
    blockbasic *ret = new blockbasic(fd);
    add_block(ret);
    return ret;
}

blockbasic* blockgraph::new_block_basic(intb offset)
{
    flowblock *b = new_block_basic();

    Address addr(fd->d->trans->getDefaultCodeSpace(), offset);
    b->set_initial_range(addr, addr);

    return b;
}

void        blockgraph::set_start_block(flowblock *bl)
{
    int i;
    if (blist[0]->flags.f_entry_point) {
        if (bl == blist[0]) return;
    }

    for (i = 0; i < blist.size(); i++)
        if (blist[i] == bl) break;

    for (; i > 0; --i)
        blist[i] = blist[i - 1];

    blist[0] = bl;
    bl->flags.f_entry_point = 1;
}

void        flowblock::set_initial_range(const Address &b, const Address &e)
{
    cover.clear();
    cover.insertRange(b.getSpace(), b.getOffset(), e.getOffset());
}

bool        flowblock::is_empty(void)
{
    pcodeop *op;
    list<pcodeop *>::iterator it;

    if (out.size() == 1 && get_out(0) == this) return false;

    if (ops.empty()) return true;

    for (it = ops.begin(); it != ops.end(); it++) {
        op = *it;
        if ((op->opcode == CPUI_BRANCH) || (op->opcode == CPUI_MULTIEQUAL)) continue;

        return false;
    }

    return true;
}

bool        flowblock::is_empty_delete(void)
{
    if (out.size() != 1) return false;
    if (get_out(0) == this) return false;

    return is_empty();
}

void        flowblock::insert(list<pcodeop *>::iterator iter, pcodeop *inst)
{
    list<pcodeop *>::iterator newiter;
    inst->parent = this;
    newiter = ops.insert(iter, inst);
    inst->basiciter = newiter;
}

int         flowblock::sub_id() 
{ 
    return index;
    if (ops.size() == 0) return 0;

    list<pcodeop *>::const_iterator iter = ops.begin();
    return (*iter)->start.getTime();
}

bool        flowblock::noreturn(void) 
{ 
    return ops.size() && last_op()->callfd && last_op()->callfd->flags.exit;  
}

pcodeop*    flowblock::get_cbranch_sub_from_cmp(void)
{
    pcodeop *lastop = last_op(), *op;
    vector<pcodeop *> q;
    varnode *in0, *in1;
    if (NULL == lastop || (lastop->opcode != CPUI_CBRANCH)) return NULL;

    op = lastop->get_in(1)->def;
    q.push_back(op);

    while (!q.empty()) {
        op = q.front();
        q.erase(q.begin());

        while (op && op->parent == this) {
            switch (op->opcode) {
            case CPUI_COPY:
            case CPUI_BOOL_NEGATE:
                op = op->get_in(0)->def;
                break;

            case CPUI_INT_NOTEQUAL:
            case CPUI_INT_EQUAL:
            case CPUI_INT_SLESS:
            case CPUI_BOOL_OR:
            case CPUI_BOOL_AND:
                in0 = op->get_in(0);
                in1 = op->get_in(1);
                if (in0->def) {
                    op = in0->def;

                    if (in1->def)
                        q.push_back(in1->def);
                }
                else
                    op = in1->def;
                break;

            case CPUI_INT_SUB:
                return op;

            default:
                op = NULL;
                break;
            }
        }
    }

    return NULL;
}

bool        flowblock::is_stack_check_fail()
{
    if (out.size()) return false;
    pcodeop *p = last_op();

    if (p->is_call() && p->flags.exit)
        return true;

    return false;
}

int         flowblock::incoming_forward_branches()
{
    int i, count = 0;

    for (i = 0; i < in.size(); i++) {
        blockedge &e = in[i];
        if (!(e.label & a_forward_edge)) continue;
        if (e.point->is_mark()) continue;

        count++;
    }

    return count;
}

bool        flowblock::is_iv_in_normal_loop(pcodeop *sub)
{
    varnode *in1 = sub->get_in(1);

    if (in1->is_constant() && (in1->get_val() >= 0) && in1->get_val() <= 1024)
        return true;

    return false;
}

bool        flowblock::is_eq_cbranch(void)
{
    pcodeop *lastop = last_op(), *def;
    if (lastop && (lastop->opcode == CPUI_CBRANCH) && (def = lastop->get_in(1)->def)
        && (def->opcode == CPUI_INT_NOTEQUAL)
        && (def->get_in(0)->get_addr() == g_dobc->get_addr("ZR"))
        && def->get_in(1)->is_constant()
        && (def->get_in(1)->get_val() == 0))
        return true;

    return false;
}

blockgraph::blockgraph(funcdata *fd1) 
{ 
    fd = fd1; 
    d = fd1->d;  
}

flowblock*  blockgraph::get_entry_point(void)
{
    int i;

    for (i = 0; i < blist.size(); i++) {
        if (blist[i]->is_entry_point())
            return blist[i];
    }

    return NULL;
}

int         flowblock::get_in_index(const flowblock *bl)
{
    int i;

    for (i = 0; i < in.size(); i++) {
        if (in[i].point == bl)
            return i;
    }

    return -1;
}

int         flowblock::get_out_index(const flowblock *bl)
{
    int i;

    for (i = 0; i < out.size(); i++) {
        if (out[i].point == bl)
            return i;
    }

    return -1;
}

void        blockgraph::calc_exitpath()
{
    flowblock *e, *in;
    int i, j;
    vector<flowblock *> q;

    for (i = 0; i < exitlist.size(); i++) {
        e = exitlist[i];

        e->flags.f_exitpath = 1;

        q.clear();
        q.push_back(e);

        while ((in = q.front())) {
            q.erase(q.begin());
            if (in->get_back_edge_count() > 0)
                continue;

            in->flags.f_exitpath = 1;

            for (j = 0; j < in->in.size(); j++) {
                q.push_back(in->get_in(j));
            }
        }
    }
}

void        blockgraph::clear(void)
{
    vector<flowblock *>::iterator iter;

    for (iter = blist.begin(); iter != blist.end(); ++iter)
        delete *iter;

    blist.clear();
}

void        blockgraph::clear_marks(void)
{
    int i;

    for (i = 0; i < blist.size(); i++)
        blist[i]->clear_mark();
}

int         blockgraph::remove_edge(flowblock *begin, flowblock *end)
{
    int i;
    for (i = 0; i < end->in.size(); i++) {
        if (end->in[i].point == begin)
            break;
    }

    return end->remove_in_edge(i);
}

void        blockgraph::add_edge(flowblock *begin, flowblock *end)
{
    end->add_in_edge(begin, 0);
}

void        blockgraph::add_edge(flowblock *begin, flowblock *end, int label)
{
    end->add_in_edge(begin, label);
}

void        flowblock::add_in_edge(flowblock *b, int lab)
{
    int outrev = b->out.size();
    int brev = in.size();
    in.push_back(blockedge(b, lab, outrev));
    b->out.push_back(blockedge(this, lab, brev));
}

int         flowblock::remove_in_edge(int slot)
{
    flowblock *b = in[slot].point;
    int label = in[slot].label;
    int rev = in[slot].reverse_index;

    half_delete_in_edge(slot);
    b->half_delete_out_edge(rev);

    return label;
}

void        flowblock::remove_out_edge(int slot)
{
    flowblock *b = out[slot].point;
    int rev = out[slot].reverse_index;

    half_delete_out_edge(slot);
    b->half_delete_in_edge(rev);
}

void        flowblock::half_delete_out_edge(int slot)
{
    while (slot < (out.size() - 1)) {
        blockedge &edge(out[slot]);
        edge = out[slot + 1];

        blockedge &edge2(edge.point->in[edge.reverse_index]);
        edge2.reverse_index -= 1;
        slot += 1;
    }

    out.pop_back();
}

void        flowblock::half_delete_in_edge(int slot)
{
    while (slot < (in.size() - 1)) {
        blockedge &edge(in[slot]);
        edge = in[slot + 1];

        blockedge &edge2(edge.point->out[edge.reverse_index]);
        edge2.reverse_index -= 1;
        slot += 1;
    }
    in.pop_back();
}

int         flowblock::get_back_edge_count(void)
{
    int i, count = 0;

    for (i = 0; i < in.size(); i++) {
        if (in[i].label & a_back_edge) count++;
    }

    return count;
}

flowblock*  flowblock::get_back_edge_node(void)
{
    int i, count = 0;

    for (i = 0; i < in.size(); i++) {
        if (in[i].label & a_back_edge) return in[i].point;
    }

    return NULL;
}

blockedge* flowblock::get_true_edge(void)
{
    pcodeop *op = last_op();
    int i;

    if (op->opcode != CPUI_CBRANCH)
        throw LowlevelError("get_true_edge() only support CPUI_CBRANCH");


    for (i = 0; i < out.size(); i++) {
        if (out[i].label & a_true_edge)
            return &out[i];
    }

    fd->dump_cfg(fd->name, "check000", 1);
    throw LowlevelError("not found true edge in flowblock");
}

blockedge*  flowblock::get_false_edge(void)
{
    pcodeop *op = last_op();
    int i;

    if (op->opcode != CPUI_CBRANCH)
        throw LowlevelError("get_false_addr() only support CPUI_CBRANCH");

    for (i = 0; i < out.size(); i++) {
        if (!(out[i].label & a_true_edge))
            return &out[i];
    }

    throw LowlevelError("not found false edge in flowblock");
}

void        flowblock::set_out_edge_flag(int i, uint4 lab)
{
    flowblock *bbout = out[i].point;
    out[i].label |= lab;
    bbout->in[out[i].reverse_index].label |= lab;
}

void        flowblock::clear_out_edge_flag(int i, uint4 lab)
{
    flowblock *bbout = out[i].point;
    out[i].label &= ~lab;
    bbout->in[out[i].reverse_index].label &= ~lab;
}

void        blockgraph::remove_from_flow(flowblock *bl)
{
    if (bl->in.size() > 0)
        throw LowlevelError("only support remove block which in-size is 0");

    flowblock *bbout;

    while (bl->out.size() > 0) {
        bbout = bl->get_out(bl->out.size() - 1);
        bl->remove_out_edge(bl->out.size() - 1);
    }
}

void        flowblock::remove_op(pcodeop *inst)
{
    inst->parent = NULL;
    ops.erase(inst->basiciter);
}

void        blockgraph::remove_block(flowblock *bl)
{
    vector<flowblock *>::iterator iter;

    if (bl->in.size())
        throw LowlevelError("only support remove block in-size is 0");

    bl->flags.f_dead = 1;

    for (iter = blist.begin(); iter != blist.end(); iter++) {
        if (*iter == bl) {
            blist.erase(iter);
            break;
        }
    }

    deadblist.push_back(bl);
}

void        blockgraph::collect_reachable(vector<flowblock *> &res, flowblock *bl, bool un) const
{
    flowblock *blk, *blk2;

    bl->set_mark();
    res.push_back(bl);
    int total = 0, i, j;

    while (total < res.size()) {
        blk = res[total++];
        for (j = 0; j < blk->out.size(); j++) {
            blk2 = blk->get_out(j);
            if (blk2->is_mark())
                continue;

            blk2->set_mark();
            res.push_back(blk2);
        }
    }

    if (un) {
        res.clear();
        for (i = 0; i < blist.size(); i++) {
            blk = blist[i];
            if (blk->is_mark())
                blk->clear_mark();
            else
                res.push_back(blk);
        }
    }
    else {
        for (i = 0; i < res.size(); i++)
            res[i]->clear_mark();
    }
}

void        blockgraph::splice_block(flowblock *bl)
{
    flowblock *outbl = (flowblock *)0;
    char f[3];
    if (bl->out.size() == 1) {
        outbl = bl->get_out(0);
        if (outbl->in.size() != 1)
            outbl = NULL;
    }
    if (outbl == NULL)
        throw LowlevelError("Can only splice block with 1 output to ");

    f[0] = bl->flags.f_entry_point;
    bl->remove_out_edge(0);

    int sizeout = outbl->out.size();
    for (int i = 0; i < sizeout; i++) 
        move_out_edge(outbl, 0, bl);

    remove_block(outbl);
    bl->flags.f_entry_point = f[0];
}

void        blockgraph::move_out_edge(flowblock *blold, int slot, flowblock *blnew)
{
    flowblock *outbl = blold->get_out(slot);
    int i = blold->get_out_rev_index(slot);
    outbl->replace_in_edge(i, blnew);
}

void        flowblock::replace_in_edge(int num, flowblock *b)
{
    flowblock *oldb = in[num].point;
    oldb->half_delete_out_edge(in[num].reverse_index);
    in[num].point = b;
    in[num].reverse_index = b->out.size();
    b->out.push_back(blockedge(this, in[num].label, num));
}

list<pcodeop *>::reverse_iterator flowblock::get_rev_iterator(pcodeop *op)
{
    list<pcodeop *>::reverse_iterator it = ops.rbegin();

    for (; it != ops.rend(); it++) {
        if (*it == op)
            return it;
    }

    throw LowlevelError("get_rev_iterator failure");
}

flowblock*  blockgraph::add_block_if(flowblock *b, flowblock *cond, flowblock *tc)
{
    add_edge(b, cond, a_true_edge);
    add_edge(b, tc);

    return b;
}

bool        blockgraph::is_dowhile(flowblock *b)
{
    int i;

    for (i = 0; i < b->out.size(); i++) {
        flowblock *o = b->get_out(i);

        if (o == b)
            return true;
    }

    return false;
}

pcodeop*    flowblock::first_callop(void)
{
    list<pcodeop *>::iterator it;

    for (it = ops.begin(); it != ops.end(); it++) {
        pcodeop *p = *it;

        if (p->callfd) return p;
    }

    return NULL;
}

pcodeop*    blockgraph::first_callop_vmp(flowblock *end)
{
    list<pcodeop *>::iterator it;
    pcodeop *op;
    dobc *d = fd->d;

    for (int i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];

        if (b == end)
            return NULL;

        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            op = *it;
            if (op->is_call() && d->test_cond_inline(d, op->get_call_offset()))
                return op;
        }
    }

    return NULL;
}

flowblock*  blockgraph::find_loop_exit(flowblock *start, flowblock *end)
{
    vector<flowblock *> stack;
    vector<int> visit;
    flowblock *b, *bb;
    int i;

    visit.resize(get_size());

    stack.push_back(start);
    visit[start->index] = 1;

    while (!stack.empty()) {
        b = stack.back();

        if (b == end) {
            do {
                stack.pop_back();
                b = stack.back();
            } while (b->out.size() == 1);

            return b;
        }

        for (i = 0; i < b->out.size(); i++) {
            bb = b->get_out(i);
            if (visit[bb->index]) continue;

            visit[bb->index] = 1;
            stack.push_back(bb);
            break;
        }

        if (i == b->out.size()) stack.pop_back();
    }

    return NULL;
}

flowblock*          blockgraph::detect_whiledo_exit(flowblock *header)
{
    flowblock *true0, *false0, *back;

    if (header->out.size() != 2)
        return NULL;

    if (header->get_back_edge_count() == 0)
        return NULL;

    back = header->get_back_edge_node();

    true0 = header->get_true_edge()->point;
    false0 = header->get_false_edge()->point;

    while (back) {
        if (back == true0) return false0;
        if (back == false0) return true0;

        back = back->immed_dom;
    }

    throw LowlevelError("loop is unreducible ?");
}

flowblock*        funcdata::combine_multi_in_before_loop(vector<flowblock *> ins, flowblock *header)
{
    int i;
    flowblock *b = bblocks.new_block_basic(user_offset += user_step);

    for (i = 0; i < ins.size(); i++) {
        int lab = bblocks.remove_edge(ins[i], header);
        bblocks.add_edge(ins[i], b, lab & a_true_edge);
    }

    bblocks.add_edge(b, header);

    clear_block_phi(header);

    structure_reset();

    return b;
}

void        funcdata::dump_exe()
{
    thumb_gen gen(this);

    gen.run();

    gen.dump();
}

void        funcdata::detect_calced_loops(vector<flowblock *> &loops)
{
    int i;
    flowblock *lheader;

    for (i = 0; i < bblocks.loopheaders.size(); i++) {
        lheader = bblocks.loopheaders[i];

        /* FIXME:硬编码了一波， */
        if ((lheader->loopnodes.size() == 2) && (lheader->dfnum < 10)) 
            loops.push_back(lheader);
    }
}

void        funcdata::remove_loop_livein_varnode(flowblock *lheader)
{
    int i;
    flowblock *o;
    list<pcodeop *>::iterator it, next;
    list<pcodeop *>::iterator itu;
    pcodeop *p, *p_use;
    varnode *out;

    /* 遍历循环内所有节点 */
    for (i = 0; i < lheader->loopnodes.size(); i++) {
        o = lheader->loopnodes[i];

        for (it = o->ops.begin(); it != o->ops.end(); it = next) {
            p = *it;
			next = ++it;

            if (!(out = p->output)) continue;

            /* 查看循环内的varnode 是否有被循环歪的节点使用到，假如是的话，则把这个节点加入到outlist中去*/
            for (itu = out->uses.begin(); itu != out->uses.end(); itu++) {
                p_use = *itu;
                if (!bblocks.in_loop(lheader, p_use->parent)) break;
            }

            if (itu == out->uses.end()) 
                op_destroy_ssa(p);
        }
    }
}

void        funcdata::remove_calculated_loop(flowblock *lheader)
{
    vector<varnode *> outlist;

    if (lheader->loopnodes.size() != 2)
        throw LowlevelError("now only support 2 nodes constant loop remove");

    flowblock *pre = loop_pre_get(lheader, 0);
    flowblock *cur = lheader, *branch;
    pcodeop *p;
    list<pcodeop *>::iterator it;
    varnode *vn;

    do {
        int inslot = cur->get_inslot(pre), ret;
        branch = NULL;

        /* 因为这个计算可计算循环，不需要把节点重新拉出来，所以不加入trace列表 */
        for (it = cur->ops.begin(); it != cur->ops.end(); it++) {
            p = *it;

            p->set_trace();
            ret = p->compute(inslot, &branch);

            if ((vn = p->output) && (vn->type.height == a_top)) {
                /* FIXME: 我这里根据某些特殊情况对load做了特殊处理，让a_top类型的load值变成了
                0xbadbeef */
                if (p->opcode == CPUI_LOAD)
                    vn->set_val(0xbadbeef);
                else if (!vn->uses.empty())
                    throw LowlevelError("in calculated loop find unkown node");
            }

#if 0
            char buf[256];
            p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
            printf("%s\n", buf);
#endif
        }

        pre = cur;
        cur = branch;
    } while (bblocks.in_loop(lheader, cur));

    remove_loop_livein_varnode(lheader);

    /* FIXME:以下的合并节点的方式不对，我简化处理了 */
    branch = lheader->loopnodes[1];
    if (!branch->ops.empty())
        throw LowlevelError("not support liveout ops live in loopsnode");

    bblocks.remove_edge(branch, lheader);
    bblocks.remove_edge(lheader, branch);
    bblocks.remove_block(branch);
    if ((lheader->out.size() == 1) && ((p = lheader->last_op())->opcode == CPUI_CBRANCH)) {
        op_destroy(p, 1);
    }

    for (it = lheader->ops.begin(); it != lheader->ops.end(); it++) {
        p = *it;
        p->clear_trace();
        vn = p->output;
        if (!vn) continue;
        if (vn->is_constant()) p->to_constant();
        if (vn->is_sp_constant()) p->to_rel_constant();
    }

    structure_reset();
}

void        funcdata::remove_calculated_loops()
{
    vector<flowblock *> loops;
    int i;

    detect_calced_loops(loops);

    for (i = 0; i < loops.size(); i++) {
        remove_calculated_loop(loops[i]);
    }

    heritage_clear();
    heritage();
}

/*

识别出这样一种结构 

top->b0
top->b1
b0->b1

b0里面只允许有1-2行代码 
*/
int         funcdata::cond_copy_propagation(varnode *phi)
{
    pcodeop *p = phi->def;
    int i;
    flowblock *b0, *b1, *top;

    if (!p 
        || (p->opcode == CPUI_MULTIEQUAL)
        || (p->inrefs.size() != 2))
        throw LowlevelError("input varnode cant cond copy propagation");

    /* 模式匹配 */
    b0 = p->parent->get_in(0);
    b1 = p->parent->get_in(1);

    if ((i = b0->get_inslot(b1)) == -1) {
        if ((i = b1->get_inslot(b0)) == -1)
            throw LowlevelError("cond_copy_propagation pattern mismatch");
        top = b1;
    }
    else
        top = b0;

    b0 = (top == b0) ? b1 : b0;
    b1 = p->parent;

    if (b0->ops.size() != 1)
        throw LowlevelError("cond_copy_propagation pattern mismatch");

    return 0;
}

int         funcdata::cond_copy_expand(pcodeop *p, flowblock *b, int outslot)
{
    flowblock *b1, *b2, *pb1, *pb2, *outb;
    vector<varnode *> defs;
    int i, have_top;
    varnode *def;
    assert(p->opcode == CPUI_COPY || p->opcode == CPUI_LOAD || p->opcode == CPUI_MULTIEQUAL);
    assert(outslot >= 0);
    outb = b->get_out(outslot);

    have_top = collect_all_const_defs(p, defs);

    if (defs.size() == 0)
        return -1;

    pb1 = pb2 = NULL;
    for (i = 0; i < defs.size(); i++) {
        def = defs[i];

        if ((def == defs.back()) && !have_top) {
            b1 = bblocks.new_block_basic(user_offset += user_step);
            pf.add_copy_const(b1, b1->ops.end(), *p->output, *defs[i]);

            bblocks.add_edge(pb1, b1);
            continue;
        }

        b1 = bblocks.new_block_basic(user_offset += user_step);
        pf.add_cmp_const(b1, b1->ops.end(), p->output, defs[i]);
        pf.add_cbranch_eq(b1);

        b2 = bblocks.new_block_basic(user_offset += user_step);
        pf.add_copy_const(b2, b2->ops.end(), *p->output, *defs[i]);

        bblocks.add_edge(b1, b2, a_true_edge);
        bblocks.add_edge(b2, outb);

        if (pb1)
            bblocks.add_edge(pb1, b1);

        pb1 = b1;
        pb2 = b2;

        if (i == 0) {
            int lab = bblocks.remove_edge(b, outb);
            bblocks.add_edge(b, b1, lab & a_true_edge);
        }
    }

    bblocks.add_edge(b1, outb);

    structure_reset();

    return 0;
}

int         funcdata::collect_all_const_defs(pcodeop *start, vector<varnode *> &defs)
{
    pcodeop *p, *def;
    pcodeop_set visit;
    pcodeop_set::iterator it;
    vector<pcodeop *> q;
    varnode *in;
    int i, j, have_top_def = 0;

    q.push_back(start);
    visit.insert(start);
    while (!q.empty()) {
        p = q.front();
        q.erase(q.begin());

#define cad_push_def(in)        do { \
            def = (in)->def; \
            if (in->is_constant()) { \
                for (j = 0; j < defs.size(); j++) { \
                    if (defs[j]->get_val() == in->get_val()) break; \
                } \
                if (j == defs.size()) defs.push_back(in); \
            } \
            else if (def && (visit.find(def) == visit.end()) && ((def->opcode == CPUI_COPY) || (def->opcode == CPUI_MULTIEQUAL) || (def->opcode == CPUI_LOAD))) { \
                q.push_back(def); \
                visit.insert(def); \
            } \
            else have_top_def = 1; \
        } while (0)

        if (p->opcode == CPUI_COPY) {
            in = p->get_in(0);
            cad_push_def(in);
        }
        else if (p->opcode == CPUI_MULTIEQUAL) {
            for (i = 0; i < p->inrefs.size(); i++) {
                in = p->get_in(i);
                cad_push_def(in);
            }
        }
        else if ((p->opcode == CPUI_LOAD) && (p->have_virtualnode())) {
            in = p->get_virtualnode();
            in = in->def->get_in(2);
            cad_push_def(in);
        }
    }

    return have_top_def;
}

int         funcdata::cut_const_defs_on_condition(pcodeop *start, vector<varnode *> &defs)
{
    flowblock *b = start->parent;
    pcodeop *p[4];

    if (((p[0] = b->last_op())->opcode == CPUI_CBRANCH)
        && (p[1] = p[0]->get_in(0)->def)
        && (p[1]->opcode == CPUI_INT_EQUAL)) {
    }

    return 0;
}

void        funcdata::rewrite_no_sub_cbranch_blk(flowblock *b)
{
    list<pcodeop *>::iterator it, it1;
    flowblock *end, *newstart;
    vector<flowblock *> cloneblks;

    end = bblocks.get_it_end_block(b);

    clear_block_phi(b);
    clear_block_phi(end);

    while (b->in.size()) {
        flowblock *inb = b->get_in(0);

        cloneblks.clear();
        newstart = clone_web(b, end, cloneblks);

        bblocks.remove_edge(inb, b);
        bblocks.add_edge(inb, newstart);
    }
}

void        funcdata::rewrite_no_sub_cbranch_blks(vector<flowblock *> &blks)
{
    int i;

    for (i = 0; i < blks.size(); i++) {
        rewrite_no_sub_cbranch_blk(blks[i]);
    }

    remove_unreachable_blocks(true, true);
    structure_reset();
    heritage_clear();
    heritage();
}


int         funcdata::ollvm_combine_lcts(pcodeop *p)
{
    assert(p->opcode == CPUI_COPY);
    varnode *in = p->get_in(0);
    flowblock *b = p->parent, *tob, *b1;
    list<pcodeop *>::iterator it;
    pcodeop *p1;
    vector<flowblock *> blks;

    if (b->out.size() != 1) return 0;

    tob = b->get_out(0);

    for (it = in->uses.begin(); it != in->uses.end(); it++) {
        p1 = *it;
        b1 = p1->parent;

        if ((b1->out.size() != 1) || (b1->get_out(0) != tob))
            continue;

        blks.push_back(b1);
    }

    if (blks.size() < 2) return 0;
    if (blks[0] == blks[1]) return 0;

    return combine_lcts(blks);
}

int         funcdata::combine_lcts(vector<flowblock *> &blks)
{
    flowblock *b = blks[0], *tob;
    vector<list<pcodeop *>::reverse_iterator> its;
    vector<pcodeop *> ops;
    pcodeop *p, *p1;
    int i, j, len, end = 0;

    its.resize(blks.size());
    tob = b->get_out(0);
    for (i = 0; i < blks.size(); i++) {
        b = blks[i];
        /* 所有节点的出口节点必须一致，且只有一个出口节点 */
        if (b->ops.size() == 0) return 0;
        if (b->out.size() != 1) return 0;
        if (b->get_out(0) != tob) return 0;

        its[i] = b->ops.rbegin();
    }

    while (!end) {
        p = *(its[0]);
        for (i = 1; i < its.size(); i++) {
            p1 = *(its[i]);

            if (p->opcode == CPUI_MULTIEQUAL) break;
            if (p->opcode != p1->opcode) break;
            if (!p->output != !p1->output) break;
            if (p->inrefs.size() != p1->inrefs.size()) break;
            if (p->output) {
                if ((d->is_temp(poa(p)) != d->is_temp(poa(p1))) || (poa(p) != poa(p1))) break;
            }

            for (j = 0; j < p->inrefs.size(); j++) {
                varnode *l = p->get_in(j);
                varnode *r = p1->get_in(j);
                bool istemp = d->is_temp(l->get_addr());
                if ((istemp == d->is_temp(r->get_addr()))) continue;
                if (!istemp && (l->get_addr() == r->get_addr()) && (l->get_size() == r->get_size())) continue;

                break;
            }
        }

        if (i == its.size())
            ops.insert(ops.begin(), p);
        else
            break;

        for (i = 0; i < its.size(); i++) {
            its[i]++;
            if (its[i] == blks[i]->ops.rend()) end = 1;
        }
    }

    if ((ops.size() == 0) || ((ops.size() == 1) && (ops.back()->opcode == CPUI_BRANCH))) return 0;

    b = bblocks.new_block_basic(user_offset += user_step);

    for (i = 0; i < ops.size(); i++) {
        Address addr2(d->get_code_space(), user_offset + p->get_addr().getOffset());
        const SeqNum sq(addr2, op_uniqid++);
        p = cloneop(p, sq);
        op_insert_end(p, b);
    }

    len = ops.size();
    while (len--) {
        for (i = 0; i < its.size(); i++) {
            its[i]--;
            op_destroy_ssa(*(its[i]));
        }
    }

    for (i = 0; i < blks.size(); i++) {
        bblocks.remove_edge(blks[i], tob);
        bblocks.add_edge(blks[i], b);
    }

    bblocks.add_edge(b, tob);

    structure_reset();

    return 1;
}

int         funcdata::cmp_itblock_cbranch_conditions(pcodeop *cbr1, pcodeop* cbr2)
{
    list<pcodeop *>::iterator it1 = cbr1->insertiter;
    list<pcodeop *>::iterator it2 = cbr2->insertiter;

    pcodeop *p1 = *--it1;
    pcodeop *pp1 = *--it1;

    pcodeop *p2 = *--it2;
    pcodeop *pp2 = *--it2;

    /* 这个复杂的表达式，只是模拟ssa的效果，保证du链，确保我们是在比较同一个东西 */
    if ((p1->opcode == CPUI_BOOL_NEGATE) && (p2->opcode == CPUI_BOOL_NEGATE) && (pp1->opcode == pp2->opcode)
        && p1->output->get_addr() == cbr1->get_in(1)->get_addr()
        && pp1->output->get_addr() == p1->get_in(0)->get_addr()
        && p2->output->get_addr() == cbr2->get_in(1)->get_addr()
        && pp2->output->get_addr() == p2->get_in(0)->get_addr()
        && (pp1->get_in(0)->get_addr() == pp2->get_in(0)->get_addr())) {
        return 0;
    }

    return -1;
}

bool        funcdata::vmp360_detect_vmeip()
{
    flowblock *vmhead = get_vmhead(), *exit = NULL;
    varnode *iv = detect_induct_variable(vmhead, exit), *pos;
    pcodeop *op, *def;
    char buf[128];

    if (!iv)
        throw LowlevelError("vmp360 not found VMEIP");

    op = vmhead->find_pcode_def(iv->get_addr());

    /**/
    for (int i = 0; i < op->inrefs.size(); i++) {
        /* 检测到归纳变量以后(也就是vmeip)，扫描它从哪个边进来的，假如不是来自于回边，直接退出 */
        if (!(vmhead->in[i].label & a_back_edge)) continue;

        def = op->get_in(i)->def;
        if (def->opcode != CPUI_LOAD) continue;
        pos = def->get_in(1);
        if (pos->type.height == a_sp_constant) {
            print_varnode(d->trans, buf, pos);

            printf("****found VMEIP [%s]\n", buf);
            vmeip = pos->get_val();
            return true;
        }
    }

    return false;
}

int         funcdata::vmp360_detect_safezone()
{
    if (vmeip == -1)
        throw LowlevelError("vmp360_detect_safezone need vmeip detect");

    /* FIXME:硬编码一波 

    */

    set_safezone(vmeip - 0x24, 0xa0);

    return 0;
}

int         funcdata::vmp360_detect_framework_info()
{
    if (!vmp360_detect_vmeip())
        throw LowlevelError("vmp360_detect_vmeip not found vmeip");

    if (vmp360_detect_safezone())
        throw LowlevelError("vmp360_detect_safezone() failure ");

    return 0;
}

int         funcdata::ollvm_deshell()
{
    int i;
    unsigned int tick = mtime_tick();
    char buf[16];
    flowblock *h;

    //flags.disable_to_const = 1;
    flags.disable_simd_to_const = 1;
    flags.disable_inrefs_to_const = 1;
    follow_flow();
    heritage();

    //dump_cfg(name, "orig", 1);
#if 1
    while (!cbrlist.empty() || !emptylist.empty()) {
        cond_constant_propagation();
        dead_code_elimination(bblocks.blist, 0);
    }
#endif

    //dump_djgraph("1", 0);

    ollvm_detect_frameworkinfo();

    dump_cfg(name, "orig", 1);

    h = ollvm_get_head();
    for (i = 0; loop_dfa_connect(0) >= 0; i++)
    {
        printf("[%s] loop_unrolling sub_%llx %d times*********************** \n\n", mtime2s(NULL),  h->get_start().getOffset(), i);
        dead_code_elimination(bblocks.blist, RDS_UNROLL0);
#if defined(DCFG_CASE)
        dump_cfg(name, _itoa(i, buf, 10), 1);
#endif
    }

    /* 删除所有splice 模块，不会导致ssa关系重构 */
    bblocks.clear_all_unsplice();
    bblocks.clear_all_vminfo();

    remove_dead_stores();
    dead_code_elimination(bblocks.blist, 0);

    do {
        cond_constant_propagation();
        dead_code_elimination(bblocks.blist, 0);
    } while (!cbrlist.empty() || !emptylist.empty());

    dead_code_elimination(bblocks.blist, F_REMOVE_DEAD_PHI);

    dump_cfg(name, "final", 1);
    dump_pcode("1");
    dump_loop("1");

	printf("deshell spent %u ms\n", mtime_tick() - tick);

    dump_exe();

    return 0;
}

int        funcdata::vmp360_deshell()
{
	unsigned int tick = mtime_tick();

    follow_flow();

    inline_call("", 1);
    inline_call("", 1);

    /* 这里需要heritage一次，才能扫描到 vmp360的框架信息，
    而没有这一次vmp360的框架信息扫描，它们后面的 heritage也是不完全的，所以在检测完360的框架以后
    需要再来一次heritage*/
    heritage();
    if (vmp360_detect_framework_info())
        throw LowlevelError("vmp360_deshell not found vmeip");

	heritage_clear();
    heritage();

    if (!cbrlist.empty())
        cond_constant_propagation();

    remove_calculated_loops();
    if (!cbrlist.empty())
        cond_constant_propagation();

	flags.enable_topstore_mark = 1;
    dump_cfg(name, "orig1", 1);

    char buf[16];
    int i;
    for (i = 0; get_vmhead(); i++) {
    //for (i = 0; i < 3; i++) {
        printf("loop unrolling %d times*************************************\n", i);
        loop_unrolling4(get_vmhead(), i, _NOTE_VMBYTEINDEX);
        dead_code_elimination(bblocks.blist, RDS_UNROLL0);
#if defined(DCFG_CASE)
        dump_cfg(name, _itoa(i, buf, 10), 1);
#endif
    }

    dump_exe();

    dump_cfg(name, "final", 1);
    dump_pcode("1");
    dump_djgraph("1", 1);
    //fd_main->dump_phi_placement(17, 5300);
    dump_store_info("1");
    dump_loop("1");

	printf("deshell spent %u ms\n", mtime_tick() - tick);

    return 0;
}

bool        flowblock::is_rel_cbranch()
{
    pcodeop *op = last_op();

    if (op && (op->opcode == CPUI_CBRANCH) && (op->get_in(0)->is_hard_constant())) return true;

    return false;
}

bool        flowblock::is_rel_branch()
{
    pcodeop *op = last_op();

    if (op && (op->opcode == CPUI_BRANCH) && (op->get_in(0)->is_hard_constant())) return true;

    return false;
}

Address    flowblock::get_return_addr()
{
    pcodeop *p = last_op();
    dobc *d = parent->fd->d;

    if ((p->opcode == CPUI_RETURN))
        return Address(d->get_code_space(), p->get_in(0)->get_val());

    if (p->output->get_addr() != d->pc_addr)
        throw LowlevelError("inline block last op output must be pc address");

    return Address(d->get_code_space(), p->output->get_val());
}

void        blockgraph::clear_all_unsplice()
{
    for (int i = 0; i < blist.size(); i++) {
        get_block(i)->flags.f_unsplice = 0;
    }
}

void        blockgraph::clear_all_vminfo()
{
    for (int i = 0; i < blist.size(); i++) {
        flowblock *b = get_block(i);
        b->vm_byteindex = -1;
        b->vm_caseindex = -1;
    }
}

bool        blockgraph::in_loop(flowblock *lheader, flowblock *node)
{
    return (node->loopheader == lheader) || (node == lheader);
}

pcodeop*    flowblock::find_pcode_def(const Address &outaddr)
{
    list<pcodeop *>::iterator it;
    pcodeop *p;

    for (it = ops.begin(); it != ops.end(); it++) {
        p = *it;
        if (p->output && p->output->get_addr() == outaddr)
            return p;
    }

    return NULL;
}

void        flowblock::dump()
{
    printf("flowblock[addr:0x%llx, index:%d, dfnum:%d]\n", get_start().getOffset(), index, dfnum);
}

void        funcdata::remove_dead_store(flowblock *b)
{
    list<pcodeop *>::iterator it, it2;
    map<valuetype, vector<pcodeop *> > m;
    pcodeop *back;

    while (b) {
        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            pcodeop *p = *it;

            if ((p->opcode != CPUI_STORE) && (p->opcode != CPUI_LOAD)) continue;

            varnode *pos = p->get_in(1);

            if (pos->type.height == a_top) {
                if (p->opcode == CPUI_LOAD)  m.clear();

                continue;
            }

            vector<pcodeop *> &stack(m[pos->type]);
            if (!stack.empty() && ((back = stack.back())->opcode == CPUI_STORE) && p->opcode == CPUI_STORE) {
                stack.pop_back();

                /* 假如发现要被删除的store，还有被使用的use，直接报错 */
                if (back->output && !back->output->has_no_use()) {
                    assert(0);
                }
                op_destroy(back, 1);
            }

            stack.push_back(p);
        }


        b = ((b->out.size() == 1) && (b->get_out(0)->in.size() == 1)) ? b->get_out(0) : NULL;
    }
}

void        funcdata::remove_dead_stores()
{
    int i;

    for (i = 0; i < bblocks.get_size(); i++) {
        remove_dead_store(bblocks.get_block(i));
    }
}

void        blockgraph::build_dom_tree(vector<vector<flowblock *> > &child)
{
    int i;
    flowblock *bl;

    child.clear();
    child.resize(blist.size() + 1);
    for (i = 0; i < blist.size(); ++i) {
        bl = blist[i];
        if (bl->immed_dom)
            child[bl->immed_dom->index].push_back(bl);
        else
            child[blist.size()].push_back(bl);
    }
}

int         blockgraph::build_dom_depth(vector<int> &depth)
{
    flowblock *bl;
    int max = 0, i;

    depth.resize(blist.size() + 1);

    for (i = 0; i < blist.size(); i++) {
        bl = blist[i]->immed_dom;
        if (bl)
            depth[i] = depth[bl->index] + 1;
        else
            depth[i] = i;

        if (max < depth[i])
            max = depth[i];
    }

    depth[blist.size()] = 0;
    return max;
}

flowblock*  blockgraph::find_post_tdom(flowblock *h)
{
    return NULL;
}

Address     flowblock::get_start(void)
{
    const Range *range = cover.getFirstRange();
    if (range == NULL) {
        assert(0);
    }

    return range->getFirstAddr();
}

/* 
Testing Flow Graph Reducibility
https://core.ac.uk/download/pdf/82032035.pdf */
bool        blockgraph::find_irreducible(const vector<flowblock *> &preorder, int &irreduciblecount)
{
    vector<flowblock *> reachunder;
    flowblock *y, *copymap, *b;
    bool needrebuild = false;
    int xi = preorder.size() - 1, i, loop, q, j;

    while (xi >= 0) {
        flowblock *x = preorder[xi];
        xi -= 1;
        int sizein = x->in.size();
        for (i = loop = 0; i < sizein; ++i) {
            if (!x->is_back_edge_in(i))
                continue;

            loop++;

            y = x->get_in(i);
            if (y == x)
                continue;

            reachunder.push_back((copymap = y->copymap));
            copymap->set_mark();

            /* 假设出现一种  */
            for (j = 0; j < copymap->irreducibles.size(); j++) {
                b = copymap->irreducibles[j];
                if (!b->is_mark()) {
                    reachunder.push_back(b);
                    b->set_mark();
                }
            }
        }

        if (loop) {
            add_loopheader(x);
        }

        q = 0;
        while (q < reachunder.size()) {
            flowblock *t = reachunder[q];
            q += 1;

            int sizein_t = t->in.size();
            for (i = 0; i < sizein_t; i++) {
                flowblock *y = t->get_in(i);
                flowblock *yprime = y->copymap;         // y' = FIND(y)

                /* 
                1. 
                2. cross edge
                */
                if ((x->dfnum > yprime->dfnum) || ((x->dfnum + x->numdesc) <= yprime->dfnum)) {
                    // FIXME:这行打印太多了，需要优化掉
                    //printf("warn: dfnum[%d] irreducible to dfnum[%d]\n", x->dfnum, yprime->dfnum);
                    //irreducibles.push_back(yprime);
                    x->irreducibles.push_back(yprime);
                    yprime->flags.f_irreducible = 1;
                }
                else if (!yprime->is_mark() && (yprime != x)) {
                    reachunder.push_back(yprime);
                    yprime->set_mark();
                }
            }
        }

        for (i = 0; i < reachunder.size(); i++) {
            flowblock *s = reachunder[i];
            s->clear_mark();
            s->copymap = x;

            s->loopheader = x;
            x->loopnodes.push_back(s);
        }

        reachunder.clear();
    }

    clear_marks();

    for (i = 0; i < preorder.size(); i++)
        preorder[i]->copymap = NULL;

    return false;
}

void        flowblock::add_op(pcodeop *op)
{
    insert(ops.end(), op);
}

funcdata::funcdata(const char *nm, const Address &a, int siz, dobc *d1)
    : startaddr(a),
    d(d1),
    bblocks(this),
    name(nm),
    alias(nm),
    searchvn(0, Address(Address::m_minimal)),
    pf(this)
{
    emitter.fd = this;

    flags.thumb = a.getOffset() & 1;
    if (flags.thumb)
        startaddr = startaddr - 1; 

    minaddr = startaddr;
    maxaddr = startaddr;

    symsize = siz;

    memstack.size = 256 * 1024;
    memstack.bottom = (u1 *)malloc(sizeof (u1) * memstack.size);
    memstack.top = memstack.bottom + memstack.size;

    vbank.uniqbase = 0x100000;
    vbank.uniqid = vbank.uniqbase;
    vbank.create_index = 1;
}

funcdata::~funcdata(void)
{
}

pcodeop*    funcdata::newop(int inputs, const SeqNum &sq)
{
    pcodeop *op = new pcodeop(inputs, sq);
    if (sq.getTime() >= op_uniqid)
        op_uniqid = sq.getTime() + 1;

    optree[op->start] = op;
    op->insertiter = deadlist.insert(deadlist.end(), op);

    return op;
}

pcodeop*    funcdata::newop(int inputs, const Address &pc)
{
    pcodeop *op = new pcodeop(inputs, SeqNum(pc, op_uniqid++));
    optree[op->start] = op;
    op->insertiter = deadlist.insert(deadlist.end(), op);

    return op;
}

varnode*    funcdata::new_varnode_out(int s, const Address &m, pcodeop *op)
{
    varnode *vn = create_def(s, m, op);
    op->output = vn;

    return vn;
}

varnode*    funcdata::new_varnode(int s, AddrSpace *base, uintb off)
{
    return new_varnode(s, Address(base, off));
}

varnode*    funcdata::new_varnode(int s, const Address &m)
{
    varnode *vn = create_vn(s, m);

    return vn;
}

varnode*    funcdata::create_vn(int s, const Address &m)
{
    varnode *vn = new varnode(s, m);

    vn->create_index = vbank.create_index++;
    vn->lociter = loc_tree.insert(vn).first;
    vn->defiter = def_tree.insert(vn).first;
    return vn;
}

varnode*    funcdata::create_def(int s, const Address &m, pcodeop *op)
{
    varnode *vn = new varnode(s, m);
    vn->create_index = vbank.create_index++;
    vn->set_def(op);

    return xref(vn);
}

varnode*    funcdata::xref(varnode *vn)
{
    pair<varnode_loc_set::iterator, bool> check;

    check = loc_tree.insert(vn);
    if (!check.second) {
        throw LowlevelError("vn already be inserted");
    }

    vn->lociter = check.first;
    vn->defiter = def_tree.insert(vn).first;

    return vn;
}

varnode*    funcdata::set_def(varnode *vn, pcodeop *op)
{
    if (!vn->is_free()) 
        throw LowlevelError("Defining varnode which is not free at " + op->get_addr().getShortcut());
    

    if (vn->is_constant())
        throw LowlevelError("Assignment to constant at ");

    loc_tree.erase(vn->lociter);
    def_tree.erase(vn->defiter);

    vn->set_def(op);

    return xref(vn);
}

varnode*    funcdata::create_def_unique(int s, pcodeop *op)
{
    Address addr(d->trans->getUniqueSpace(), vbank.uniqid);

    vbank.uniqid += s;

    return create_def(s, addr, op);
}

varnode*    funcdata::create_constant_vn(intb val, int size)
{
    Address addr(d->trans->getConstantSpace(), val);

    return create_vn(size, addr);
}

void        funcdata::op_set_opcode(pcodeop *op, OpCode opc)
{
    if (opc)
        remove_from_codelist(op);
    op->set_opcode(opc);
    add_to_codelist(op);
}

void        funcdata::op_resize(pcodeop *op, int size)
{
    op->inrefs.resize(size);
}

void        funcdata::op_set_input(pcodeop *op, varnode *vn, int slot)
{
    while (op->inrefs.size() < (slot + 1))
        op->inrefs.push_back(NULL);

    if (vn == op->get_in(slot))
        return; 

    if (vn->is_constant()) {
    }

    if (op->get_in(slot))
        op_unset_input(op, slot);

    vn->add_use(op);
    op->inrefs[slot] = vn;
}

void        funcdata::op_set_output(pcodeop *op, varnode *vn)
{
    if (vn == op->get_out())
        return;

    if (op->get_out())
        op_unset_output(op);

    if (vn->get_def())
        op_unset_output(vn->get_def());

    vn = set_def(vn, op);

    op->set_output(vn);
}

void        funcdata::op_unset_input(pcodeop *op, int slot)
{
    varnode *vn = op->get_in(slot);

    vn->del_use(op);
    op->clear_input(slot);
}

void        funcdata::op_unset_output(pcodeop *op)
{
    varnode *vn = op->get_out();

    if (vn == NULL) return;
    op->set_output(NULL);
    mark_free(vn);
}

void        funcdata::op_remove_input(pcodeop *op, int slot)
{
    op_unset_input(op, slot);
    op->remove_input(slot);
}

void        funcdata::op_insert_input(pcodeop *op, varnode *vn, int slot)
{
    op->insert_input(slot);
    op_set_input(op, vn, slot);
}

void        funcdata::op_zero_multi(pcodeop *op)
{
    if (op->num_input() == 0) {
        assert(0);
    }
    else if (op->num_input() == 1) {
        op_set_opcode(op, CPUI_COPY);
        op->flags.copy_from_phi = 1;
    }
}

void        funcdata::op_unlink(pcodeop *op)
{
    int i;

    op_unset_output(op);

    for (i = 0; i < op->inrefs.size(); i++)
        op_unset_input(op, i);
    if (op->parent)
        op_uninsert(op);
}

void        funcdata::op_uninsert(pcodeop *op)
{
    mark_dead(op);
    op->parent->remove_op(op);
}

void        funcdata::clear_block_phi(flowblock *b)
{
    pcodeop *p;
    list<pcodeop *>::iterator it, next;

#if 0
    while ((p = b->ops.front())) {
        if ((p->opcode != CPUI_MULTIEQUAL) && !p->flags.copy_from_phi) break;

        op_destroy_ssa(p);
    }
#else
    for (it = b->ops.begin(); it != b->ops.end(); it = next) {
        p = *it;
        next = ++it;

        if (p->opcode == CPUI_MULTIEQUAL) {
            op_destroy_ssa(p);
            continue;
        }

        if (p->flags.copy_from_phi) continue;

        break;
    }
#endif
}

void        funcdata::clear_block_df_phi(flowblock *b)
{
    vector<flowblock *>     blks;
    int i;

    blks.push_back(b);
    calc_phi_placement3(blks);

    for (i = 0; i < merge.size(); i++) {
        flowblock *m = merge[i];

        clear_block_phi(m);
    }
}


/* 1. 返回这个地址对应的instruction的第一个pcode的地址
   2. 假如这个地址上的instruction没有产生pcode，返回顺序的下一个instruction的首pcode的地址 */
pcodeop*    funcdata::target(const Address &addr) const
{
    map<Address, VisitStat>::const_iterator iter;

    iter = visited.find(addr);
    while (iter != visited.end()) {
        const SeqNum &seq(iter->second.seqnum);
        if (!seq.getAddr().isInvalid()) {
            pcodeop *retop = find_op(seq);
            if (retop)
                return retop;
            break;
        }

        iter = visited.find(iter->first + iter->second.size);
    }

    throw LowlevelError("Could not find op at target address");
}

/* 设置当前解析地址到哪个地址为止
假如发现 栈顶 位置已经被访问过了，弹出当前地址，并返回
假如发现 栈顶 位置小于某个已访问到的地址b，把地址b设置为边界
假如发现 栈顶 位置就是访问过的地址中最大的，设置eaddr为bound
*/
bool        funcdata::set_fallthru_bound(Address &bound)
{
    map<Address, VisitStat>::const_iterator iter;
    const Address &addr(addrlist.back());

    iter = visited.upper_bound(addr);
    if (iter != visited.begin()) {
        --iter;
        if (addr == iter->first) {
            addrlist.pop_back();
            pcodeop *op = target(addr);
            op->flags.startblock = 1;
            return false;
        }
        /* 这个是对同一个地址有不同的解析结果，在一些保护壳中有用 */
        if (addr < (iter->first + iter->second.size)) {
            throw LowlevelError("different interpreted instruction in address");
        }

        ++iter;
    }

    if (iter != visited.end())
        bound = iter->first;
    else
        bound = eaddr;

    return true;
}

pcodeop*    funcdata::find_op(const Address &addr)
{
    map<Address, VisitStat>::iterator iter;
    iter = visited.find(addr);
    if (iter == visited.end()) return NULL;
    return find_op(iter->second.seqnum);
}

pcodeop*    funcdata::find_op(const SeqNum &num) const
{
    pcodeop_tree::const_iterator iter = optree.find(num);
    if (iter == optree.end()) return NULL;
    return iter->second;
}

int         funcdata::inst_size(const Address &addr)
{
    map<Address, VisitStat>::iterator iter;
    iter = visited.find(addr);
    return iter->second.size;
}

/* 
Algorithms for Computing the Static Single Assignment Form

GIANFRANCO BILARDI

https://www.cs.utexas.edu/users/pingali/CS380C/2007fa/papers/ssa.pdf

up-edge: 
*/
void        funcdata::build_adt(void)
{
    flowblock *x, *v, *u;
    int i, j, k, l;
    int size = bblocks.get_size();
    vector<int>     a(size);
    vector<int>     b(size, 0);
    vector<int>     t(size, 0);
    vector<int>     z(size, 0);

    /*
    Definition 4. Given a CFG G = (V, E), (u -> v) in E is an up-edge if
u != idom(v). The subgraph (V, Eup) of G containing only the up-edges is called
the a-DF graph.
*/
    vector<flowblock *> upstart, upend;

    augment.clear();
    augment.resize(size);
    phiflags.clear();
    phiflags.resize(size, 0);

    bblocks.build_dom_tree(domchild);
    maxdepth = bblocks.build_dom_depth(domdepth);
    for (i = 0; i < size; i++) {
        x = bblocks.get_block(i);
        for (j = 0; j < domchild[i].size(); ++j) {
            v = domchild[i][j];
            for (k = 0; k < v->in.size(); ++k) {
                u = v->get_in(k);
                if (u != v->immed_dom) {
                    upstart.push_back(u);
                    upend.push_back(v);
                    b[u->index] += 1;
                    t[x->index] += 1;
                }
            }
        }
    }

    for (i = size - 1; i >= 0; --i) {
        k = 0;
        l = 0;
        for (j = 0; j < domchild[i].size(); ++j) {
            k += a[domchild[i][j]->index];
            l += z[domchild[i][j]->index];
        }

        a[i] = b[i] - t[i] + k;
        z[i] = 1 + l;

        if ((domchild[i].size() == 0) || (z[i] > (a[i] + 1))) {
            phiflags[i] |= boundary_node;
            z[i] = 1;
        }
    }
    z[0] = -1;
    for (i = 1; i < size; ++i) {
        j = bblocks.get_block(i)->immed_dom->index;
        if (phiflags[j] & boundary_node)
            z[i] = j;
        else
            z[i] = z[j];
    }

    for (i = 0; i < upstart.size(); ++i) {
        v = upend[i];
        j = v->immed_dom->index;
        k = upstart[i]->index;
        while (j < k) {
            augment[k].push_back(v);
            k = z[k];
        }
    }
}

/*
某些指令会生成内部跳转指令，比如
        0002565e 63 f9 0f 28     vld2.8     {d18,d19},[param_4]
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

里面的cbranch实际上都在指令里面跳,这个时候它的in[0]，实际上是一个相对于当前位置的常数，上图的似乎被修正过了，
实际上是这样:

pcode27:    cbranch 4, 0

那么pcode27实际上跳往的地址是pcode31
*/
pcodeop*    funcdata::find_rel_target(pcodeop *op, Address &res) const
{
    const Address &addr(op->get_in(0)->get_addr());
    uintm id = op->start.getTime() + addr.getOffset();
    SeqNum seqnum(op->start.getAddr(), id);
    pcodeop *retop = find_op(seqnum);
    if (retop)
        return retop;

    SeqNum seqnum1(op->get_addr(), id - 1);
    retop = find_op(seqnum1);
    if (retop) {
        map<Address, VisitStat>::const_iterator miter;
        miter = visited.upper_bound(retop->get_addr());
        if (miter != visited.begin()) {
            --miter;
            res = miter->first + miter->second.size;
            if (op->get_addr() < res)
                return NULL;
        }
    }

    throw LowlevelError("Bad relative branch at instruction");
}

/* 当扫描某个branch指令时，查看他的to地址，是否已经再visited列表里，假如已经再的话

把目的地址设置为startblock，假如不是的话，推入扫描地址列表 

*/
void     funcdata::new_address(pcodeop *from, const Address &to)
{
    if (visited.find(to) != visited.end()) {
        pcodeop *op = target(to);
        op->flags.startblock = 1;
        return;
    }

    addrlist.push_back(to);
}

void        funcdata::del_varnode(varnode *vn)
{
}

varnode_loc_set::const_iterator     funcdata::begin_loc(const Address &addr)
{
    searchvn.loc = addr;
    return loc_tree.lower_bound(&searchvn);
}

varnode_loc_set::const_iterator     funcdata::end_loc(const Address &addr)
{
    if (addr.getOffset() == addr.getSpace()->getHighest()) {
        AddrSpace *space = addr.getSpace();
        searchvn.loc = Address(d->trans->getNextSpaceInOrder(space), 0);
    }
    else
        searchvn.loc = addr + 1;

    return loc_tree.lower_bound(&searchvn);
}

varnode_loc_set::const_iterator     funcdata::begin_loc(AddrSpace *spaceid)
{
    searchvn.loc = Address(spaceid, 0);
    return loc_tree.lower_bound(&searchvn);
}

varnode_loc_set::const_iterator     funcdata::end_loc(AddrSpace *spaceid)
{
    searchvn.loc = Address(d->trans->getNextSpaceInOrder(spaceid), 0);
    return loc_tree.lower_bound(&searchvn);
}

void        funcdata::del_op(pcodeop *op)
{
    int i;
    for (i = 0; i < op->inrefs.size(); i++) {
    }
}

/* 当一个指令内产生了多个pcode以后，比如

inst1: p1, p2, p3, p4, p5

p2产生了跳转，p3, p4, p5全部都要删除，到下一个指令为止
*/
void        funcdata::del_remaining_ops(list<pcodeop *>::const_iterator oiter)
{
    while (oiter != deadlist.end()) {
        pcodeop *op = *oiter;
        ++oiter;
        del_op(op);
    }
}

void        funcdata::add_callspec(pcodeop *p, funcdata *fd)
{
    Address *addr;
    varnode *vn;
    p->callfd = fd;
    qlst.push_back(new func_call_specs(p, fd));

    for (int i = 0; i < d->argument_regs.size(); i++) {
        addr = d->argument_regs[i];
        if (!p->get_in(*addr)) {
            vn = new_varnode(4, *addr);
            op_set_input(p, vn, p->inrefs.size());
        }
    }

    if (!p->get_in(d->lr_addr)) {
        vn = new_varnode(4, d->lr_addr);
        op_set_input(p, vn, p->inrefs.size());
    }

    if (!p->get_in(d->sp_addr)) {
        vn = new_varnode(4, d->sp_addr);
        op_set_input(p, vn, p->inrefs.size());
    }

    /* 我们认为函数一定会修改 r0 的值，不管它有无返回 */
    if (!p->output) {
        varnode *vn = new_varnode(4, d->r0_addr);
        op_set_output(p, vn);
    }
}

pcodeop*    funcdata::xref_control_flow(list<pcodeop *>::const_iterator oiter, bool &startbasic, bool &isfallthru)
{
    funcdata *fd;
    pcodeop *op = NULL;
    isfallthru = false;
    uintm maxtime = 0;
    int exit = 0;

    while (oiter != deadlist.end()) {
        op = *oiter++;
        if (startbasic) {
            op->flags.startblock = 1;
            startbasic = false;
        }

        switch (op->opcode) {
        case CPUI_CBRANCH:
        case CPUI_BRANCH: {
            const Address &destaddr(op->get_in(0)->get_addr());
            startbasic = true;

            /* 没看懂，destaddr指向了常量空间? */
            if (destaddr.isConstant()) {
                Address fallThruAddr;
                pcodeop *destop = find_rel_target(op, fallThruAddr);
                if (destop) {
                    destop->flags.startblock = 1;
                    uintm newtime = destop->start.getTime();
                    if (newtime > maxtime)
                        maxtime = newtime;
                }
                else
                    isfallthru = true;
            }
            /*
在某些加壳程序中, branch指令会模拟call的效果，它会branch到一个函数里面去，这个时候
我们需要检查目的地址是否是某个函数的地址，假如是的话，就不加入扫描列表，同时把这个
branch指令打上call_branch标记

它不同于一般的call，一般的call指令，还有隐含的把pc压入堆栈行为，call完还会回来，
但是branch跳过去就不回来了
            */
            else if ((fd = d->find_func(destaddr))) {
                if (op->opcode == CPUI_CBRANCH)
                    throw LowlevelError("not support cbranch on " + fd->name);

                startbasic = false;
                op->flags.branch_call = 1;
                op->flags.exit = 1;

                add_callspec(op, fd);
            }
            else
                new_address(op, destaddr);
        }
            break;

        case CPUI_BRANCHIND:
            if (op->get_in(0)->is_constant()) {
                Address destaddr(d->trans->getConstantSpace(), op->get_in(0)->get_val());
                new_address(op, destaddr);
            }
            else
                tablelist.push_back(op);
        case CPUI_RETURN:

            if (op->start.getTime() >= maxtime) {
                del_remaining_ops(oiter);
                oiter = deadlist.end();
            }
            startbasic = true;
            break;

        case CPUI_CALL: 
        {
                const Address &destaddr(op->get_in(0)->get_addr());
                /* 假如发现某些call的函数有exit标记，则开始新的block，并删除当前inst的接下去的所有pcode，因为走不到了 */
                if ((fd = d->add_func(destaddr))) {
                    if (exit = fd->flags.exit) startbasic = true;

                    add_callspec(op, fd);
                }
                op->flags.exit = exit;
        }
        break;

        case CPUI_CALLIND:
			add_callspec(op, NULL);
            break;
        }

        if ((op->opcode == CPUI_BRANCH
            || op->opcode == CPUI_BRANCHIND
            || op->opcode == CPUI_RETURN
            || exit) && (op->start.getTime() >= maxtime)) {
            del_remaining_ops(oiter);
            oiter = deadlist.end();
        }
    }

    if (exit)
        isfallthru = false;
    else if (isfallthru)
        startbasic = true;
    else if (!op)
        isfallthru = true;
    else {
        switch (op->opcode) {
        case CPUI_BRANCH:
        case CPUI_BRANCHIND:
        case CPUI_RETURN:
            break;

        default:
            isfallthru = true;
            break;
        }
    }

    return op;
}

bool        funcdata::process_instruction(const Address &curaddr, bool &startbasic)
{
    bool emptyflag;
    bool isfallthru = true;
    AssemblyRaw assem;

    list<pcodeop *>::const_iterator oiter;
    int step;

    if (inst_count >= inst_max) {
        throw LowlevelError("Flow exceeded maximum allowable instruction");
    }

    inst_count++;

    /* 这里做了一堆花操作，其实就是让 oiter，一定要指向instruction生成的第一个pcode */
    if (optree.empty())
        emptyflag = true;
    else {
        emptyflag = false;
        oiter = deadlist.end();
        --oiter;
    }

    d->context->setVariable("TMode", curaddr, flags.thumb);

    if (d->context->getVariable("condit", curaddr)) {
        emitter.enter_itblock();
    }
    else {
        emitter.exit_itblock();
    }

    if (flags.dump_inst)
        d->trans->printAssembly(assem, curaddr);

    assem.set_mnem(1);
    d->trans->printAssembly(assem, curaddr);
    
    step = d->trans->oneInstruction(emitter, curaddr);

    VisitStat &stat(visited[curaddr]);
    stat.size = step;

    if (curaddr < minaddr)
        minaddr = curaddr;
    if (maxaddr < (curaddr + step))
        maxaddr = curaddr + step;

    if (emptyflag)
        oiter = deadlist.begin();
    else
        ++oiter;

    /* 这个时候oiter指向的是新生成的instruction的第一个pcode的位置, 这个判断是为了防止
    某些instruction没有生成pcode */
    if (oiter != deadlist.end()) {
        stat.seqnum = (*oiter)->start;
        (*oiter)->flags.startinst = 1;

        xref_control_flow(oiter, startbasic, isfallthru);
    }

    if (isfallthru)
        addrlist.push_back(curaddr + step);

    return isfallthru;
}

/* 指令有跳转和无跳转之分，fallthru的意思就是直达往下，我们从一个地址开始分析，
 假如碰到条件跳转指令，把要跳转的地址压入，然后继续往下分析，一直到无法往下为止，如碰到return
*/
void        funcdata::fallthru()
{
    Address bound;

    /* 设置 直达 边界 */
    if (!set_fallthru_bound(bound))
        return;

    Address curaddr;
    bool startbasic = true;
    bool fallthruflag;

    while (!addrlist.empty()) {
        curaddr = addrlist.back();
        addrlist.pop_back();
        fallthruflag = process_instruction(curaddr, startbasic);
        if (!fallthruflag)
            break;

        if (bound <= addrlist.back()) {
            if (bound == addrlist.back()) {
                if (startbasic) {
                    pcodeop *op = target(addrlist.back());
                    op->flags.startblock = 1;
                }

                addrlist.pop_back();
                break;
            }

            if (!set_fallthru_bound(bound))
                return;
        }
    }
}

jmptable*   funcdata::find_jmptable(pcodeop *op)
{
    vector<jmptable *>::const_iterator iter;
    jmptable *jt;

    for (iter = jmpvec.begin(); iter != jmpvec.end(); ++iter) {
        jt = *iter;
        if (jt->opaddr == op->get_addr()) return jt;
    }

    return NULL;
}

void        funcdata::recover_jmptable(pcodeop *op, int elmsize)
{
    Address addr(op->start.getAddr());
    jmptable *newjt = new jmptable(op);

    int i;

    for (i = 0; i < (elmsize + 2); i++) {
        addrlist.push_back(addr + 4 + 4 * i);

        newjt->addresstable.push_back(addr + 4 + 4 * i);
    }
    newjt->defaultblock = elmsize + 1;
    jmpvec.push_back(newjt);
}

void        funcdata::fix_jmptable()
{
    int i, j;
    flowblock *bb;

    for (i = 0; i < jmpvec.size(); i++) {
        jmptable *jt = jmpvec[i];
        for (j = 0; j < jt->addresstable.size(); j++) {
            Address &addr = jt->addresstable[j];
            pcodeop *op = find_op(addr);

            if (!op->flags.startblock)
                throw LowlevelError("indirect jmp not is start block");

            bb = op->parent;
            bb->jmptable = jt;
            bb->flags.f_switch_out = 1;
        }

        jt->op->parent->type = a_switch;
        jt->op->parent->jmptable = jt;
    }
}

void        funcdata::analysis_jmptable(pcodeop *op)
{
    varnode *vn = op->get_in(0);
    pcodeop *def = vn->def;
    int reg, reg1;
    unsigned int data, data1;

    if (d->trans->getRegisterName(vn->loc.getSpace(), vn->loc.getOffset(), vn->loc.getAddrSize()) == "pc") {
        d->loader->loadFill((uint1 *)&data, 4, op->start.getAddr());
        d->loader->loadFill((uint1 *)&data1, 4, op->start.getAddr() - 4);
        
#define ARM_ADD_MASK        0x00800000
#define ARM_CMP_MASK        0x03500000

        if ((data & ARM_ADD_MASK) == ARM_ADD_MASK
            && (data1 & ARM_CMP_MASK) == ARM_CMP_MASK) {
            reg = data & 0xf;
            reg1 = (data1 >> 16) & 0xf;
            if (reg == reg1) {
                recover_jmptable(op, data1 & 0xfff);
                return;
            }
        }
        /* 不是switch table */
    }
}

void        funcdata::generate_ops_start()
{
    addrlist.push_back(startaddr );

    generate_ops();
}

void        funcdata::generate_ops(void)
{
    vector<pcodeop *> notreached;       // 间接跳转是不可达的?

    /* 修改了原有逻辑，可以多遍op_generated*/
    //printf("%s generate ops %d times\n", name.c_str(), op_generated+1);

    while (!addrlist.empty())
        fallthru();

    while (!tablelist.empty()) {
        pcodeop *op = tablelist.back();
        tablelist.pop_back();

        analysis_jmptable(op);

        while (!addrlist.empty())
            fallthru();
    }

    op_generated++;
}

pcodeop*    funcdata::branch_target(pcodeop *op) 
{
    const Address &addr(op->get_in(0)->get_addr());

    if (addr.isConstant()) {
        Address res;
        pcodeop *retop = find_rel_target(op, res);
        if (retop)
            return retop;

        return target(res);
    }

    return target(addr);
}

pcodeop*    funcdata::fallthru_op(pcodeop *op)
{
    pcodeop*    retop;
    list<pcodeop *>::const_iterator iter = op->insertiter;
    ++iter;
    if (iter != deadlist.end()) {
        retop = *iter;
        if (!retop->flags.startinst)
            return retop;
    }

    map<Address, VisitStat>::const_iterator miter;
    miter = visited.upper_bound(op->get_addr());
    if (miter == visited.begin())
        return NULL;
    --miter;
    if ((*miter).first + (*miter).second.size <= op->get_addr())
        return NULL;
    return target((*miter).first + (*miter).second.size);
}

void        funcdata::collect_edges()
{
    list<pcodeop *>::const_iterator iter, iterend;
    list<op_edge *>::const_iterator iter1;
    jmptable *jt;
    pcodeop *op, *target_op, *target_op1;
    bool nextstart;
    int i;

    if (flags.blocks_generated)
        throw LowlevelError(name + "blocks already generated");

    iter = deadlist.begin();
    iterend = deadlist.end();
    for (; iter != iterend; ) {
        op = *iter++;
        if (iter == iterend)
            nextstart = true;
        else
            nextstart = (*iter)->flags.startblock;

        switch (op->opcode) {
        case CPUI_BRANCH:
            if (!op->flags.branch_call) {
                target_op = branch_target(op);
                block_edge.push_back(new op_edge(op, target_op));
            }
            break;

        case CPUI_BRANCHIND:
            if (op->get_in(0)->is_constant()) {
                Address addr(d->get_code_space(), op->get_in(0)->get_val());

                target_op = find_op(addr);
                if (!target_op)
                    throw LowlevelError("not found address ");
                block_edge.push_back(new op_edge(op, target_op));
                break;
            }

            jt = find_jmptable(op);
            if (jt == NULL) break;

            for (i = 0; i < jt->addresstable.size(); i++) {
                target_op = target(jt->addresstable[i]);
                if (target_op->flags.mark)
                    continue;
                target_op->flags.mark = 1;

                block_edge.push_back(new op_edge(op, target_op));
            }

            iter1 = block_edge.end();
            while (iter1 != block_edge.begin()) {
                --iter1;
                if ((*iter1)->from == op)
                    (*iter1)->to->flags.mark = 0;
                else
                    break;
            }
            break;

        case CPUI_RETURN:
            break;

        case CPUI_CBRANCH:
            if (op->flags.itblock) {
                list<pcodeop *>::const_iterator it = iter, next_it;

                for (; (it != deadlist.end()) && (*it)->flags.itblock; it = next_it) {
                    pcodeop *p = *it;
                    next_it = ++it;

                    if (p->opcode == CPUI_CBRANCH) {
                        if (cmp_itblock_cbranch_conditions(op, p))
                            break;
                        else {
                            op_unset_input(op, 0);
                            op_set_input(op, clone_varnode(p->get_in(0)), 0);
                            op_destroy_raw(p);
                        }
                    }
                }
            }

            target_op = fallthru_op(op);
            block_edge.push_back(new op_edge(op, target_op));

            target_op1 = branch_target(op);
            block_edge.push_back(new op_edge(op, target_op1));
            block_edge.back()->t = 1;
            break;

        default:
            if (op->flags.exit)
                break;

            if (nextstart) {
                target_op = fallthru_op(op);
                block_edge.push_back(new op_edge(op, target_op));
            }
            break;
        }

    }
}

void        funcdata::mark_alive(pcodeop *op)
{
    op->flags.dead = 0;
}

void        funcdata::mark_dead(pcodeop *op)
{
    op->flags.dead = 1;
}

void        funcdata::mark_free(varnode *vn)
{
    loc_tree.erase(vn->lociter);
    def_tree.erase(vn->defiter);

    vn->set_def(NULL);
    vn->flags.insert = 0;
    vn->flags.input = 0;

    vn->lociter = loc_tree.insert(vn).first;
    vn->defiter = def_tree.insert(vn).first;
}

void        funcdata::op_insert(pcodeop *op, blockbasic *bl, list<pcodeop *>::iterator iter)
{
    mark_alive(op);
    bl->insert(iter, op);
}

void        funcdata::op_insert_begin(pcodeop *op, blockbasic *bl)
{
    list<pcodeop *>::iterator iter = bl->ops.begin();

    if (op->opcode != CPUI_MULTIEQUAL) {
        while (iter != bl->ops.end()) {
            if ((*iter)->opcode != CPUI_MULTIEQUAL)
                break;

            ++iter;
        }
    }

    op_insert(op, bl, iter);
}

void        funcdata::op_insert_end(pcodeop *op, blockbasic *bl)
{
    list<pcodeop *>::iterator iter = bl->ops.end();

    if (iter != bl->ops.begin()) {
        //--iter;
    }

    op_insert(op, bl, iter);
}

void        funcdata::connect_basic()
{
    flowblock *from, *to;
    op_edge *edge;
    list<op_edge *>::const_iterator iter;

    iter = block_edge.begin();
    while (!block_edge.empty()) {
        edge = block_edge.front();
        block_edge.erase(block_edge.begin());
        from = edge->from->parent;
        to = edge->to->parent;

        bblocks.add_edge(from, to, edge->t ? a_true_edge:0);

        //printf("0x%x -> 0x%x\n", (int)edge->from->start.getAddr().getOffset(), (int)edge->to->start.getAddr().getOffset());
    }
}

void        funcdata::split_basic()
{
    pcodeop *op;
    blockbasic *cur;
    list<pcodeop *>::const_iterator iter, iterend;

    iter = deadlist.begin();
    iterend = deadlist.end();
    if (iter == iterend)
        return;

    op = *iter++;

    cur = bblocks.new_block_basic();
    op_insert(op, cur, cur->ops.end());
    bblocks.set_start_block(cur);

    Address start = op->get_addr();
    Address stop = start;

    while (iter != iterend) {
        op = *iter++;

        if (op->flags.startblock) {
            cur->set_initial_range(start, stop);
            cur = bblocks.new_block_basic();
            start = op->start.getAddr();
            stop = start;
        }
        else {
            const Address &nextaddr(op->get_addr());
            if (stop < nextaddr)
                stop = nextaddr;
        }

        op_insert(op, cur, cur->ops.end());
    }
    cur->set_initial_range(start, stop);
}

void        funcdata::generate_blocks()
{
    clear_blocks();

    collect_edges();
    split_basic();
    connect_basic();

    if (bblocks.blist.size()) {
        flowblock *startblock = bblocks.blist[0];
        if (startblock->in.size()) {
            // 保证入口block没有输入边
            blockbasic *newfront = bblocks.new_block_basic();
            bblocks.add_edge(newfront, startblock);
            bblocks.set_start_block(newfront);
            newfront->set_initial_range(startaddr, startaddr);
        }
    }

    fix_jmptable();

    flags.blocks_generated = 1;

    op_gen_iter = --deadlist.end();

    structure_reset();
}

void        funcdata::dump_inst()
{
    map<Address, VisitStat>::iterator it, prev_it;
    AssemblyRaw assememit;
    char buf[128];
    FILE *fp;

    sprintf(buf, "%s/%s/inst.txt", d->filename.c_str(), name.c_str());
    fp = fopen(buf, "w");

    assememit.set_fp(fp);

    for (it = visited.begin(); it != visited.end(); it++) {
        if (it != visited.begin()) {
            /* 假如顺序的地址，加上size不等于下一个指令的地址，代表中间有缺漏，多打印一个 空行 */
            if ((prev_it->first + prev_it->second.size) != it->first) {
                fprintf(fp, "\n");
            }
        }

        d->trans->printAssembly(assememit, it->first);

        prev_it = it;
    }

    fclose(fp);
}

void        funcdata::dump_pcode(const char *postfix)
{
    FILE *fp;
    char buf[8192];
    Address prev_addr;
    AssemblyRaw assememit;
    pcodeop *p;

    sprintf(buf, "%s/pcode_%s.txt", get_dir(buf), postfix);
    fp = fopen(buf, "w");

    list<pcodeop *>::iterator iter = deadlist.begin();

    assememit.set_fp(fp);

    for (int i = 0; i < d->trans->numSpaces(); i++) {
        AddrSpace *spc = d->trans->getSpace(i);
        fprintf(fp, "Space[%s, type:%d, 0x%llx, %c]\n", spc->getName().c_str(), spc->getType(), spc->getHighest(), spc->getShortcut());
    }
    fprintf(fp, "ma = mult_addr\n");
    fprintf(fp, "\n");

    for (; iter != deadlist.end(); iter++) {
        p = *iter;
        if (p->flags.dead) continue;

        if ((p->opcode == CPUI_MULTIEQUAL) || p->flags.copy_from_phi) {
            fprintf(fp, "<tr>"
                "<td><font color=\"" COLOR_ASM_STACK_DEPTH "\">000</font></td>"
                "<td><font color=\"" COLOR_ASM_ADDR "\">0000</font></td>"
                "<td align=\"left\"><font color=\"" COLOR_ASM_INST_MNEM "\">phi--------------</font></td>"
                "<td align=\"left\"><font color=\"" COLOR_ASM_INST_BODY "\"></font></td></tr>");
        }
        else if (p->flags.startinst) {
            d->trans->printAssembly(assememit, p->get_dis_addr());
        }

        p->dump(buf, PCODE_DUMP_ALL & ~PCODE_HTML_COLOR);
        fprintf(fp, "%s\n", buf);

        prev_addr = (*iter)->get_dis_addr();
    }

    fclose(fp);
}

void        funcdata::dump_block(FILE *fp, blockbasic *b, int flag)
{
    Address prev_addr;
    pcodeop *p;
    list<pcodeop *>::iterator iter;
    AssemblyRaw assem;
    char obuf[2048];

    assem.set_buf(obuf);

    // 把指令都以html.table的方式打印，dot直接segment fault了，懒的调dot了，不再用png, jpg改用svg
    if (b->vm_byteindex >= 0) {
        fprintf(fp, "loc_%x [style=\"filled\" fillcolor=%s label=<<table bgcolor=\"white\" align=\"left\" border=\"0\">"
            "<tr><td><font color=\"green\">sub_%llx(%d,%d, h:%d, vbi:%d, vci:%d)</font></td></tr>",
            b->sub_id(),
            block_color(b),
            b->get_start().getOffset(),
            b->dfnum,
            b->index, domdepth.size() ? domdepth[b->index]:0,
            b->vm_byteindex,
            b->vm_caseindex);
    }
    else {
        fprintf(fp, "loc_%x [style=\"filled\" fillcolor=%s label=<<table bgcolor=\"white\" align=\"left\" border=\"0\">"
                    "<tr><td><font color=\"red\">sub_%llx(df:%d, ind:%d, domh:%d, outl:%d, looph_df:%d)</font></td></tr>",
            b->sub_id(),
            block_color(b),
            b->get_start().getOffset(),
            b->dfnum,
            b->index, domdepth.size() ?  domdepth[b->index]:0, b->is_out_loop(), b->loopheader ? b->loopheader->dfnum:0);
    }

    iter = b->ops.begin();

    for (p = NULL;  iter != b->ops.end() ; iter++) {
        p = *iter;

        const Address &disaddr = p->get_dis_addr();

        //if (p->flags.startinst) 
        if ((prev_addr != disaddr) && !disaddr.isInvalid())
        {
            assem.set_sp(p->sp);
            d->trans->printAssembly(assem, disaddr);
            fprintf(fp, "%s", obuf);
        }

        if (flag) {
            p->dump(obuf, PCODE_DUMP_SIMPLE | PCODE_HTML_COLOR);
            fprintf(fp, "<tr><td></td><td></td><td colspan=\"2\" align=\"left\">%s</td><td></td></tr>", obuf);
        }

        prev_addr = disaddr;
    }
    fprintf(fp, "</table>>]\n");
}

void        funcdata::dump_djgraph(const char *postfix, int flag)
{
    char obuf[512];
    flowblock *child;

    sprintf(obuf, "%s/djgraph_%s.dot", get_dir(obuf), postfix);

    FILE *fp = fopen(obuf, "w");
    if (NULL == fp) {
        printf("fopen failure %s", obuf);
        exit(0);
    }

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    int i, j;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        blockbasic *b = bblocks.blist[i];

        dump_block(fp, b, flag);
    }

    for (i = 0; i < (domchild.size() - 1); i++) {
        flowblock *dom = bblocks.get_block(i);

        for (j = 0; j < domchild[i].size(); j++) {
            child = domchild[i][j];
            fprintf(fp, "loc_%x ->loc_%x [color=\"red\" penwidth=3]\n", dom->sub_id(), child->sub_id());
        }

        for (j = 0; j < dom->out.size(); j++) {
            child = dom->get_out(j);
            if (child->immed_dom != dom)
                fprintf(fp, "loc_%x ->loc_%x [label = \"J\" color=\"blue\" penwidth=2]\n",
                    dom->sub_id(), child->sub_id());
        }
    }

    clear_blocks_mark();

    fprintf(fp, "}");
    fclose(fp);
}

char*       funcdata::edge_color(blockedge *e)
{
    if (e->label & a_tree_edge)     return "red";
    if (e->label & a_back_edge)     return "blue";
    if (e->label & a_cross_edge)    return "black";
    if (e->label & a_forward_edge)  return "green";

    return "black";
}

int         funcdata::edge_width(blockedge *e)
{
    if (e->label & a_tree_edge)     return 1;
    if (e->label & a_back_edge)     return 1;
    if (e->label & a_cross_edge)    return 1;
    if (e->label & a_forward_edge)  return 1;

    return 1;
}

char*       funcdata::block_color(flowblock *b)
{
    list<pcodeop *>::iterator iter = b->ops.end();
    if (b->flags.f_entry_point)     return "red";

    /* 出口节点 */
    if (b->out.empty())             return "blue";
    if (b->irreducibles.size())     return "deeppink";
    if (b->flags.f_loopheader)      return "green";

    return "white";
}

void        funcdata::dump_rank(FILE *fp)
{
    int i, j, k;
    vector<vector<flowblock *>> ranks;

    ranks.resize(bblocks.blist.size());

    for (i = 0; i < bblocks.blist.size(); i++) {
        flowblock *b = bblocks.blist[i];
        ranks[domdepth[b->index]].push_back(b);
    }


    fprintf(fp, "{ rank = same; ");

    for (i = k = 0; i < ranks.size(); i++) {
        if (ranks[i].size() == 0) break;

        for (j = 0; j < ranks[i].size(); j++) {
            fprintf(fp, "loc_%x; ", ranks[i][j]->sub_id());

            if (++k % 50 == 0)
                fprintf(fp, "}\n { rank = same; ");
        }
    }

    fprintf(fp, "}\n");
}

void        funcdata::dump_cfg(const string &name, const char *postfix, int dumppcode)
{
    char obuf[512];

    sprintf(obuf, "%s/cfg_%s_%s.dot", get_dir(obuf), name.c_str(), postfix);

    FILE *fp = fopen(obuf, "w");
    if (NULL == fp) {
        printf("fopen failure %s", obuf);
        exit(0);
    }

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    int i, j, k;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        blockbasic *b = bblocks.blist[i];

        dump_block(fp, b, dumppcode);
    }

    int size = bblocks.blist.size();
    for (i = 0, k = 0; i < size; ++i) {
        blockbasic *b = bblocks.blist[i];

        for (j = 0; j < b->out.size(); ++j, k++) {
            blockedge *e = &b->out[j];

            fprintf(fp, "loc_%x ->loc_%x [label = \"%s\" color=\"%s\" penwidth=%d]\n",
                b->sub_id(), e->point->sub_id(),  e->is_true() ? "true":"false", edge_color(e), edge_width(e));
        }
    }

    if (k > 600)
        dump_rank(fp);

    fprintf(fp, "}");

    fclose(fp);
}

void        funcdata::dump_loop(const char *postfix)
{
    char obuf[512];

    sprintf(obuf, "%s/loop_%s.dot", get_dir(obuf), postfix);

    FILE *fp = fopen(obuf, "w");
    if (NULL == fp) {
        printf("fopen failure %s", obuf);
        exit(0);
    }

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    int i;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        blockbasic *b = bblocks.blist[i];

        dump_block(fp, b, 0);
    }

    blockbasic *loop_header;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        blockbasic *b = bblocks.blist[i];
        loop_header = b->loopheader ? b->loopheader : bblocks.get_block(0);

        /* loopnodes 有包括自己的头节点，需要省略掉 */
        if (b->loopheader && (b->loopheader != b)) 
            fprintf(fp, "loc_%x ->loc_%x [label = \"%s\"]\n",
                b->sub_id(), loop_header->sub_id(), "");
    }

    fprintf(fp, "}");

    fclose(fp);
}

void        funcdata::dump_liverange(const char *postfix)
{
    char obuf[8192];

    sprintf(obuf, "%s/liverange_%s.txt", get_dir(obuf), postfix);

    FILE *fp = fopen(obuf, "w");
    if (NULL == fp) {
        printf("fopen failure %s", obuf);
        exit(0);
    }

	list<pcodeop *>::iterator it = deadlist.begin();

	for (; it != deadlist.end(); it++) {
		pcodeop *p = *it;
		if (p->is_dead() || !p->output) continue;

		p->output->dump_cover(obuf);

		fprintf(fp, "p%d %s\n", p->start.getTime(), obuf);
	}

    fclose(fp);
}

varnode*    funcdata::new_coderef(const Address &m)
{
    varnode *vn;

    vn = new_varnode(1, m);
    vn->flags.annotation = 1;
    return vn;
}

varnode*    funcdata::new_unique(int s)
{
    Address addr(d->get_uniq_space(), vbank.uniqid);

    vbank.uniqid += s;

    return create_vn(s, addr);
}

varnode*    funcdata::new_unique_out(int s, pcodeop *op)
{
    varnode* vn = create_def_unique(s, op);

    op->set_output(vn);

    return vn;
}

varnode*    funcdata::clone_varnode(const varnode *vn)
{
    varnode *newvn = new_varnode(vn->size, vn->loc);

    newvn->flags.annotation = vn->flags.annotation;
    newvn->flags.readonly = vn->flags.readonly;
    newvn->flags.virtualnode = vn->flags.virtualnode;
    newvn->flags.from_pc = vn->flags.from_pc;
    if (newvn->flags.from_pc)
        newvn->type = vn->type;

    return newvn;
}

void        funcdata::destroy_varnode(varnode *vn)
{
    list<pcodeop *>::const_iterator iter;

    for (iter = vn->uses.begin(); iter != vn->uses.end(); ++iter) {
        pcodeop *op = *iter;
        /* 
        1. clear_block_phi在清理multi节点的use时，会把out的use中被处理过的置空
        */
        if (!op) continue;

        op->clear_input(op->get_slot(vn));
    }

    if (vn->def) {
        vn->def->output = NULL;
        vn->def = NULL;
    }

    vn->uses.clear();
    delete_varnode(vn);
}

void        funcdata::delete_varnode(varnode *vn)
{
    if (vn->def) {
        printf("warn:try to remove varnode have def[%d] forbidden. %s:%d\n", vn->def->start.getTime(), __FILE__,__LINE__);
        return;
    }

    loc_tree.erase(vn->lociter);
    def_tree.erase(vn->defiter);
    delete(vn);
}

varnode*    funcdata::set_input_varnode(varnode *vn)
{
    varnode *v1;
    if (vn->flags.input) return vn;

    /* 假如发现有调用者，尝试从调用者中获取参数值，当执行 argument_inline 时需要 */
    if (caller && (v1 = callop->get_in(vn->get_addr()))) {
        vn->type = v1->type;
    }
    else if (vn->get_addr() == d->sp_addr) {
        vn->set_sp_constant(0);
    }

    vn->flags.input = 1;
    return vn;
}

pcodeop*    funcdata::cloneop(pcodeop *op, const SeqNum &seq)
{
    int i, sz;
    sz = (op->opcode == CPUI_LOAD) ? 2:op->inrefs.size();

    pcodeop *newop1 = newop(sz, seq);
    op_set_opcode(newop1, op->opcode);

    newop1->flags.startinst = op->flags.startinst;
    newop1->flags.startblock = op->flags.startblock;
	newop1->flags.uncalculated_store = op->flags.uncalculated_store;
    /* 我们有时候会给store分配一个虚拟的varnode节点，不要拷贝它 */
    if (op->output && (op->opcode != CPUI_STORE))
        op_set_output(newop1, clone_varnode(op->output));
    
    for (i = 0; i < sz; i++)
        op_set_input(newop1, clone_varnode(op->get_in(i)), i);

    newop1->callfd = op->callfd;
    newop1->disaddr = new Address (op->get_dis_addr());

    return newop1;
}

void        funcdata::op_destroy_raw(pcodeop *op)
{
    int i;

	for (i = 0; i < op->inrefs.size(); i++) {
		if (op->get_in(i))
			destroy_varnode(op->get_in(i));
	}
    if (op->output)
        destroy_varnode(op->output);

    optree.erase(op->start);
    deadlist.erase(op->insertiter);
	remove_from_codelist(op);
}

void        funcdata::op_destroy(pcodeop *op)
{
    int i;
    flowblock *b;

	if (op->output) {
        destroy_varnode(op->output);
		op->output = NULL;
	}

    for (i = 0; i < op->num_input(); ++i) {
        varnode *vn = op->get_in(i);
        if (vn)
            op_unset_input(op, i);
    }

    if ((b = op->parent)) {
        mark_dead(op);
        op->parent->remove_op(op);

        if (b->is_empty_delete())
            emptylist.push_back(b);
    }
}

void		funcdata::op_destroy(pcodeop *op, int remove)
{
	op_destroy(op);
	if (remove)
		op_destroy_raw(op);
}

void        funcdata::reset_out_use(pcodeop *p)
{
    varnode *out = p->output;
    if (!out)
        return;

    list<pcodeop *>::iterator it;
    list<pcodeop *> copy = out->uses;
    pcodeop *use;
    int slot;

    it = copy.begin();
    for (; it != copy.end(); it++) {
        use = *it;
        if (use == p) continue;

        slot = use->get_slot(out);
        assert(slot >= 0);

        if ((p->opcode == CPUI_COPY) && poa(p) == pi0a(p))
            op_set_input(use, pi0(p), slot);
        else
            op_set_input(use, new_varnode(out->size, out->get_addr()), slot);
    }
}

void        funcdata::op_destroy_ssa(pcodeop *p)
{
    reset_out_use(p);
    op_destroy(p, 1);
}

void        funcdata::total_replace(varnode *vn, varnode *newvn)
{
    list<pcodeop *>::const_iterator iter;
    pcodeop *op;
    int i;

    iter = vn->uses.begin();
    while (iter != vn->uses.end()) {
        op = *iter++;
        i = op->get_slot(vn);
        op_set_input(op, newvn, i);
    }
}

void		funcdata::remove_all_dead_op()
{
	list<pcodeop *>::iterator it, next;
	pcodeop *p;

	for (it = deadlist.begin(); it != deadlist.end(); it = next) {
		p = *it;
		next = ++it;
		if (!p->flags.dead) continue;

		op_destroy_raw(p);
	}
}

void        funcdata::inline_clone(funcdata *inlinefd, const Address &retaddr)
{
    list<pcodeop *>::const_iterator iter;

    for (iter = inlinefd->deadlist.begin(); iter != inlinefd->deadlist.end(); ++iter) {
        pcodeop *op = *iter;
        pcodeop *cloneop1;
        if ((op->opcode == CPUI_RETURN) && !retaddr.isInvalid()) {
            cloneop1 = newop(1, op->start);
            op_set_opcode(cloneop1, CPUI_BRANCH);
            varnode *vn = new_coderef(retaddr);
            op_set_input(cloneop1, vn, 0);
        }
        else
            cloneop1 = cloneop(op, op->start);
    }

    visited.insert(inlinefd->visited.begin(), inlinefd->visited.end());
}

void        funcdata::inline_call(string name, int num)
{
    list<func_call_specs *>::iterator iter = qlst.begin();

    for (; iter != qlst.end(); iter++) {
        if (name.empty() || (*iter)->get_name() == name) {
            inline_call((*iter)->get_addr(), num);
            break;
        }
    }
}

void        funcdata::inline_call(const Address &addr, int num)
{
    int del;
    while (num--) {
        list<func_call_specs *>::iterator iter = qlst.begin();

        for (del = 0; iter != qlst.end(); iter++) {
            func_call_specs *cs = *iter;
            if ((*iter)->get_addr() == addr) {
                qlst.erase(iter);
                inline_flow(cs->fd, cs->op);
                delete cs;
                del = 1;
                break;
            }
        }

        if (!del && (iter == qlst.end()))
            break;
    }
}

void        funcdata::inline_ezclone(funcdata *fd, const Address &calladdr)
{
    list<pcodeop *>::const_iterator iter;
    for (iter = fd->deadlist.begin(); iter != fd->deadlist.end(); iter++) {
        pcodeop *op = *iter;
        if (op->opcode == CPUI_RETURN)
            break;

        // 这里原先的Ghidra把inline的所有地址都改成了calladdr里的地址，这个地方我不理解，
        // 这里inline函数的地址统统不改，使用原先的
        //SeqNum seq(calladdr, op->start.getTime());
        SeqNum seq(op->start);
        cloneop(op, seq);
    }

    visited.insert(fd->visited.begin(), fd->visited.end());
}

void        funcdata::inline_flow(funcdata *fd1, pcodeop *callop)
{
    funcdata fd(fd1->name.c_str(), fd1->get_addr(), fd1->symsize, fd1->d);
    pcodeop *firstop;

    if (callop->flags.inlined)
        throw LowlevelError("callop already be inlined");

    callop->flags.inlined = 1;

    /* inline的fd，从父函数里的op_uniq开始，这样可以保证再inline后，uniq是连续的 */
    fd.set_op_uniqid(get_op_uniqid());
    fd.set_range(fd1->baddr, fd1->eaddr);
    fd.generate_ops_start();

    if (fd.check_ezmodel()) {
        list<pcodeop *>::const_iterator oiter = deadlist.end();
        --oiter;
        inline_ezclone(&fd, callop->get_addr());
        ++oiter;    // 指向inline的第一个pcode

        if (oiter != deadlist.end()) {
            firstop = *oiter;
            oiter = deadlist.end();
            --oiter; // 指向inline的最后一个pcode
            pcodeop *lastop = *oiter;
            if (callop->flags.startblock)
                firstop->flags.startblock = 1;
            else
                firstop->flags.startblock = 0;
        }
        /* FIXME:对单block的节点，要不要也加上对inline节点的跳转，在后期的merge阶段合并掉？ */
        op_destroy_raw(callop);
    }
    else {
        Address retaddr;
        if (!test_hard_inline_restrictions(&fd, callop, retaddr))
            return;

        inline_clone(&fd, retaddr);

        vector<jmptable *>::const_iterator iter;
        for (iter = fd.jmpvec.begin(); iter != fd.jmpvec.end(); iter++) {
            jmptable *jtclone = new jmptable(*iter);
            jmpvec.push_back(jtclone);
            jtclone->update(this);
        }

        while (callop->num_input() > 1)
            op_remove_input(callop, callop->num_input() - 1);

        if (callop->output) {
            reset_out_use(callop);
            destroy_varnode(callop->output);
        }

        op_set_opcode(callop, CPUI_BRANCH);
        varnode *inlineaddr = new_coderef(fd.get_addr());
        op_set_input(callop, inlineaddr, 0);

        callop->callfd = NULL;
    }

    /* qlst用来存放所有函数call的链表，当inline一个函数时，需要把它的call列表inline进来 */
    list<func_call_specs *>::const_iterator it = fd.qlst.begin();
    for (; it != fd.qlst.end(); it++) {
        pcodeop *op = find_op((*it)->op->start);
        func_call_specs *cs = new func_call_specs(op, (*it)->fd);

        qlst.push_back(cs);
    }

    generate_blocks();
}

flowblock*      funcdata::argument_inline(funcdata *inlinefd, pcodeop *callop)
{
    funcdata fd(inlinefd->name.c_str(), inlinefd->get_addr(), 0, d);
    flowblock *pblk = callop->parent;
    bool ez = false;

    if (callop->flags.inlined)
        throw LowlevelError("callop already be inlined");

    printf("try cond inline p%d[%llx, %s]\n", callop->start.getTime(), callop->callfd->get_addr().getOffset(), callop->callfd->name.c_str());

    /* inline的fd，从父函数里的op_uniq开始，这样可以保证再inline后，uniq是连续的 */
    fd.set_caller(this, callop);
    fd.set_op_uniqid(get_op_uniqid());
    fd.set_user_offset(get_user_offset());
    fd.set_virtualbase(get_virtualbase());
    fd.set_range(inlinefd->baddr, inlinefd->eaddr);
    fd.generate_ops_start();
    fd.generate_blocks();
    fd.heritage();


#if defined(DCFG_COND_INLINE)
    char buf[32];
    sprintf(buf, "cond_before");
    fd.dump_cfg(fd.name, buf, 1);
#endif

    fd.argument_pass();

#if defined(DCFG_COND_INLINE)
    sprintf(buf, "cond_after");
    fd.dump_cfg(fd.name, buf, 1);
#endif

    flowblock *b = fd.bblocks.get_block(0);
    list<pcodeop *>::iterator it = b->ops.begin();

    /* 把condline后的首blk的头节点都复制到 callop的后面*/
	if (fd.bblocks.get_size() < 5) {
		for (; it != b->ops.end(); it++) {
			pcodeop *p = *it;
			pcodeop *op;

			if (p->opcode == CPUI_RETURN) break;

			Address addr2(d->get_code_space(), user_offset + p->get_addr().getOffset());
			SeqNum seq(addr2, op_uniqid++);
			op = cloneop(p, seq);

			op_insert(op, callop->parent, callop->basiciter);
		}
	}

    if (fd.bblocks.get_size() == 3) {
        flowblock *orig_true_b = b->get_true_edge()->point;
        flowblock *orig_false_b = b->get_false_edge()->point;
        flowblock *true_b = clone_block(b->get_true_edge()->point, F_OMIT_RETURN);
        flowblock *false_b = clone_block(b->get_false_edge()->point, F_OMIT_RETURN);

        flowblock *tailb = split_block(pblk, callop->basiciter);
        flowblock *out = pblk->get_out(0), *b;

        while (pblk->out.size() > 0) {
            out = pblk->get_out(0);
            int lab = bblocks.remove_edge(pblk, out);
            bblocks.add_edge(tailb, out, lab);
        }

        /* 3个节点的cbranch有2种结构 
        1. a->b a->c
        2. a->b a->c b->c
        */
        bblocks.add_block_if(pblk, true_b, false_b);
        if (orig_true_b->out.empty() && orig_false_b->out.empty()) {
            if (orig_true_b->get_return_addr() == (callop->get_dis_addr() + 4))
                bblocks.add_edge(true_b, tailb);

            if (orig_false_b->get_return_addr() == (callop->get_dis_addr() + 4))
                bblocks.add_edge(false_b, tailb);
        }
        else {
            b = orig_false_b->out.size() ? orig_true_b:orig_false_b;
            if (orig_false_b->out.size())
                bblocks.add_edge(false_b, true_b);
            else
                bblocks.add_edge(true_b, false_b);

            if (b->get_return_addr() == (callop->get_dis_addr() + 4))
                bblocks.add_edge((b == orig_true_b) ? true_b:false_b, tailb);
        }

        true_b->flags.f_cond_cbranch = 1;
        false_b->flags.f_cond_cbranch = 1;
    }
	else if (fd.bblocks.get_size() == 5) {
		inline_call(callop, &fd);
	}
    else if (fd.bblocks.get_size() != 1) {
        throw LowlevelError("only support one or three block inline" );
    }

    flowblock *p = callop->parent;

    op_destroy_ssa(callop);

    structure_reset();

    heritage_clear();
    heritage();

    return p;
}


void        funcdata::argument_pass(void)
{
    int changed = 1;
    pcodeop *p;
    flowblock *b;
    vector<flowblock *> v;

    int g_time = 0;

    while (changed) {
        changed = 0;

        if (!cbrlist.empty() || !emptylist.empty()) {
            cond_constant_propagation();
            /* 发生了条件常量传播以后，整个程序的结构发生了变化，整个结构必须得重来 */
            changed = 1;
            dead_code_elimination(bblocks.blist, RDS_0);
			flags.enable_topstore_mark = 1;

            continue;
        }

        if ((p = bblocks.first_callop_vmp(NULL))) {
            changed = 1;
            /* FIXME:这里判断的过于粗糙，还需要判断整个p->parent是否再循环内*/
            if (bblocks.is_dowhile(p->parent)) {
                v.clear();
                v.push_back(p->parent);
                dowhile2ifwhile(v);
                p = bblocks.first_callop_vmp(NULL);
                heritage_clear();
                heritage();
            }

            argument_inline(p->callfd, p);
            g_time++;

            continue;
        }

        if (!bblocks.loopheaders.empty()) {
            b = bblocks.loopheaders.front();
            bblocks.loopheaders.erase(bblocks.loopheaders.begin());

            changed = 1;
            if (bblocks.is_dowhile(b)) {
                v.clear();
                v.push_back(b);
                dowhile2ifwhile(v);

                heritage_clear();
                heritage();
            }
        }
    }

    heritage_clear();
    heritage();

    dead_code_elimination(bblocks.blist, RDS_0);
}

void        funcdata::set_caller(funcdata *caller1, pcodeop *callop1)
{
    caller = caller1;
    callop = callop1;
}

bool        funcdata::check_ezmodel(void)
{
    list<pcodeop *>::const_iterator iter = deadlist.begin();

    while (iter != deadlist.end()) {
        pcodeop *op = *iter;
        //if (op->flags.call || op->flags.branch)
        if ((op->opcode == CPUI_BRANCH) || (op->opcode == CPUI_CBRANCH))
            return false;

        ++iter;
    }

    return true;
}

/* 对于当前的cfg，重新计算 loop结构和支配关系 
cfg发生改变以后，整个loop结构和支配关系都需要重新计算，
并且一次改变，可能需要重新计算多次
*/
void funcdata::structure_reset()
{
    vector<flowblock *> rootlist;

    flags.blocks_unreachable = 0;
	reset_version++;

    bblocks.structure_loops(rootlist);
    bblocks.calc_forward_dominator(rootlist);
    //bblocks.calc_exitpath();
}

void        funcdata::clear_blocks()
{
    if (!flags.blocks_generated)
        return;

    bblocks.clear();

    flags.blocks_generated = 0;
}

void        funcdata::clear_blocks_mark()
{
    int i;

    for (i = 0; i < bblocks.blist.size(); i++)
        bblocks.blist[i]->flags.f_mark = 0;
}

void        funcdata::follow_flow(void)
{
    char buf[128];
    sprintf(buf, "%s/%s", d->filename.c_str(), name.c_str());
    mdir_make(buf);

    Address baddr(d->get_code_space(), 0);
    Address eaddr(d->get_code_space(), ~((uintb)0));
    set_range(baddr, eaddr);

    generate_ops_start();
    generate_blocks();
}

void        funcdata::start_processing(void)
{
    flags.processing_started = 1;
}

void        funcdata::visit_incr(flowblock *qnode, flowblock *vnode)
{
    int i, j, k;
    flowblock *v, *child;
    vector<flowblock *>::iterator   iter, enditer;

    i = vnode->index;
    j = qnode->index;
    iter = augment[i].begin();
    enditer = augment[i].end();

    for (; iter != enditer; ++iter) {
        v = *iter;
        if (v->immed_dom->index < j) {
            k = v->index;
            if ((phiflags[k] & merged_node) == 0) {
                merge.push_back(v);
                phiflags[k] |= merged_node;
            }

            if ((phiflags[k] & mark_node) == 0) {
                phiflags[k] |= mark_node;
                pq.insert(v, domdepth[k]);
            }
        }
        else
            break;
    }

    if ((phiflags[i] && boundary_node) == 0) {
        for (j = 0; j < domchild[i].size(); ++j) {
            child = domchild[i][j];
            if ((phiflags[child->index] & mark_node) == 0)
                visit_incr(qnode, child);
        }
    }
}


bool        funcdata::trace_push(pcodeop *op)
{
    flowblock *bb;
    list<pcodeop *>::const_iterator it;

    if (trace.empty()) {
        bb = op->parent;

        for (it = bb->ops.begin(); it != bb->ops.end(); it++) {
            trace_push_op(*it);
        }
    }
    else
        trace_push_op(op);

    return true;
}

void        funcdata::trace_push_op(pcodeop *op)
{
    trace.push_back(op);
}

void        funcdata::trace_clear()
{
    trace.clear();
}

pcodeop*    funcdata::trace_store_query(pcodeop *load)
{
    vector<pcodeop *>::reverse_iterator it;
    pcodeop *p, *maystore = NULL;
    varnode *in, *vn = load->get_in(1);

    if (trace.empty()) 
        return store_query(load, NULL, vn, &maystore);

    for (it = trace.rbegin(); it != trace.rend(); it++) {
        p = *it;
        if (p->opcode == CPUI_CALL) return NULL;
        if (p->opcode == CPUI_STORE) {
            in = p->get_in(1);
            if (in->type == vn->type)
                return p;
        }
    }

    return store_query(NULL, p->parent, vn, &maystore);
}

#if 0
pcodeop*    funcdata::store_query(pcodeop *load, flowblock *b, varnode *pos, pcodeop **maystore)
{
    list<pcodeop *>::reverse_iterator it1;
    list<pcodeop *>::iterator it;

    flowblock *bb;
    pcodeop *p, *tmpstore;

    if (load) {
        it = load->basiciter;
        b = load->parent;
    }
    else {
        it = b->ops.end();
    }

    while (1) {
       while (it != b->ops.begin()) {
            it--;
            p = *it;

            if (!p->flags.inlined && b->fd->have_side_effect(p, pos)) {
                return NULL;
            }
            if (p->in_sp_alloc_range(pos)) return p;
            if (p->opcode != CPUI_STORE) continue;
			if (p->flags.uncalculated_store) continue;

            varnode *a = p->get_in(1);

            if (a->type.height == a_top) {
                if (b->fd == this) *maystore = p;
                return NULL;
            }

            if (a->type == pos->type)
                return p;
        }

        if (b->is_entry_point()) {
            if (b->fd->caller) {
                pcodeop *p1 = b->fd->callop;
                b =  p1->parent;
                it = p1->basiciter;
                // skip , FIXME:没有处理当这个op已经为第一个的情况
            }
            else
                break;
        }
        else if (b->in.size() > 1){
            flowblock *dom = b->immed_dom;
            vector<flowblock *> stack;
            vector<int> visited;

            visited.clear();
            visited.resize(b->fd->bblocks.get_size());

            tmpstore = NULL;
            for (int i = 0; i < b->in.size(); i++) {
                if (b->get_in(i) == dom) continue;
                stack.push_back(b->get_in(i));
            }

            while (!stack.empty()) {
                b = stack.front();
                stack.erase(stack.begin());

                if (visited[b->dfnum])
                    continue;

                visited[b->dfnum] = 1;

                for (it1 = b->sideeffect_ops.rbegin(); it1 != b->sideeffect_ops.rend(); it1++) {
                    p = *it1;

                    if (have_side_effect(p, pos))
                        return NULL;

                    if (p->opcode == CPUI_STORE) {
                        varnode *a = p->get_in(1);

                        /* 在分支中找到了store节点，假如是第一个就保存起来，
                        假如不是第一个，则比较是否相等，不是的话返回NULL */
                        if (a->is_top()) {
#if 0
                            if (load && load->output->maystore_from_this(p))
                                continue;
#endif

                            *maystore = p;
                            return NULL;
                        }

                        if (a->type == pos->type) {
#if 0
                            if (NULL == tmpstore)
                                tmpstore = p;
                            else if (tmpstore->get_in(2) != p->get_in(2))
                                return NULL;
#else
                            return NULL;
#endif
                        }
                    }
                }

                for (int i = 0; i < b->in.size(); i++) {
                    bb = b->get_in(i);
                    if (bb == dom)
                        continue;

                    stack.push_back(bb);
                }
            }

            if (tmpstore)
                return tmpstore;

            b = dom;
            it = b->ops.end();
       }
        else {
            b = b->get_in(0);
            it = b->ops.end();
        }
    }

    return NULL;
}
#else
pcodeop*    funcdata::store_query(pcodeop *load, flowblock *b, varnode *pos, pcodeop **maystore)
{
    list<pcodeop *>::reverse_iterator it;

    flowblock *bb;
    pcodeop *p, *tmpstore;

    if (load) {
        b = load->parent;

        for (it = b->sideeffect_ops.rbegin(); it != b->sideeffect_ops.rend(); it++) {
            if (load->start.getOrder() > (*it)->start.getOrder())
                break;
        }
    }
    else {
        it = b->sideeffect_ops.rbegin();
    }

    while (1) {
        for (; it != b->sideeffect_ops.rend(); it++) {
            p = *it;

            if (!p->flags.inlined && b->fd->have_side_effect(p, pos)) {
                return NULL;
            }
            if (p->in_sp_alloc_range(pos)) return p;
            if (p->opcode != CPUI_STORE) continue;
			if (p->flags.uncalculated_store) continue;

            varnode *a = p->get_in(1);

            if (a->type.height == a_top) {
                if (b->fd == this) *maystore = p;
                return NULL;
            }

            if (a->type == pos->type)
                return p;
        }

        if (b->is_entry_point()) {
            if (b->fd->caller) {
                pcodeop *p1 = b->fd->callop;
                b =  p1->parent;

                for (it = b->sideeffect_ops.rbegin(); it != b->sideeffect_ops.rend(); it++) {
                    if (load->start.getOrder() > (*it)->start.getOrder())
                        break;
                }
                // skip , FIXME:没有处理当这个op已经为第一个的情况
            }
            else
                break;
        }
        else if (b->in.size() > 1){
            flowblock *dom = b->immed_dom;
            vector<flowblock *> stack;
            vector<int> visited;

            visited.clear();
            visited.resize(b->fd->bblocks.get_size());

            tmpstore = NULL;
            for (int i = 0; i < b->in.size(); i++) {
                if (b->get_in(i) == dom) continue;
                stack.push_back(b->get_in(i));
            }

            while (!stack.empty()) {
                b = stack.front();
                stack.erase(stack.begin());

                if (visited[b->dfnum])
                    continue;

                visited[b->dfnum] = 1;

                for (it = b->sideeffect_ops.rbegin(); it != b->sideeffect_ops.rend(); it++) {
                    p = *it;

                    if (have_side_effect(p, pos))
                        return NULL;

                    if (p->opcode == CPUI_STORE) {
                        varnode *a = p->get_in(1);

                        /* 在分支中找到了store节点，假如是第一个就保存起来，
                        假如不是第一个，则比较是否相等，不是的话返回NULL */
                        if (a->is_top()) {
#if 0
                            if (load && load->output->maystore_from_this(p))
                                continue;
#endif

                            *maystore = p;
                            return NULL;
                        }

                        if (a->type == pos->type) {
#if 0
                            if (NULL == tmpstore)
                                tmpstore = p;
                            else if (tmpstore->get_in(2) != p->get_in(2))
                                return NULL;
#else
                            return NULL;
#endif
                        }
                    }
                }

                for (int i = 0; i < b->in.size(); i++) {
                    bb = b->get_in(i);
                    if (bb == dom)
                        continue;

                    stack.push_back(bb);
                }
            }

            if (tmpstore)
                return tmpstore;

            b = dom;
            it = b->sideeffect_ops.rbegin();
        }
        else {
            b = b->get_in(0);
            it = b->sideeffect_ops.rbegin();
        }
    }

    return NULL;
}
#endif

bool        funcdata::loop_unrolling4(flowblock *h, int vm_caseindex, uint32_t flags)
{
    int meet_exit, i, inslot;
    flowblock *cur = loop_unrolling(h, h, flags, meet_exit);
    vector<flowblock *> blks;
    pcodeop *p;
    char buf[32];

    cur->vm_caseindex = vm_caseindex;

    if (meet_exit) return true;

    if (flags & _DUMP_ORIG_CASE)
        return 0;

#if defined(DCFG_BEFORE)
    sprintf(buf, "%d_orig", vm_caseindex);
    dump_cfg(name, buf, 1);
#endif

    vector<flowblock *> stack;
    vector<flowblock *> v;
    flowblock *b, *bb, *c;
    list<pcodeop *>::iterator it;
    varnode *iv, *vn, *vn1;

    cur->mark_unsplice();
    stack.push_back(cur);


    while ((stack.back() != h) && !stack.empty()) {
        b = stack.back();

#if 1
        blks.clear();
        collect_blocks_to_node(blks, b, h);
        inslot = h->get_inslot(blks[0]);
        iv = detect_induct_variable(h, c);
        vn = iv->def->get_in(inslot);

        if ((blks.size() == 1) && vn->is_constant()) 
            break;

        if ((blks.size() > 1) && vn->is_constant()) {
            for (i = 1; i < blks.size(); i++) {
                inslot = h->get_inslot(blks[i]);
                vn1 = iv->def->get_in(inslot);

                if (!vn1->is_constant() || (vn1->get_val() != vn->get_val()))
                    break;
            }

            if (i == blks.size()) {
                bb = combine_multi_in_before_loop(blks, h);
                heritage_clear();
                heritage();
                stack.push_back(bb);
                continue;
            }
        }
#endif

        if (b->out.empty()) {
            b->flags.f_cond_cbranch = 0;
            stack.pop_back();
            b->set_mark();
        }
        else if (b->get_back_edge_count()) {
        /* 是循环节点，进行循环展开 */
#if defined(DCFG_AFTER_PCODE)
            loop_unrolling(b, h, _DUMP_PCODE | _DONT_CLONE, meet_exit);
#else
            loop_unrolling(b, h, _DONT_CLONE, meet_exit);
#endif
            stack.pop_back();
#if defined(DCFG_AFTER)
            dump_cfg(name, "check2", 1);
#endif
        }
        else if (b->flags.f_cond_cbranch) {
            b->flags.f_cond_cbranch = 0;
#if defined(DCFG_AFTER_PCODE)
            loop_unrolling(b, h, _DUMP_PCODE | _DONT_CLONE, meet_exit);
#else
            loop_unrolling(b, h, _DONT_CLONE, meet_exit);
#endif
            stack.pop_back();
#if defined(DCFG_AFTER)
            dump_cfg(name, "check1", 1);
#endif
        }
        else if (p = get_vmcall(b)) {
            argument_inline(p->callfd, p);

            b->mark_unsplice();
            if (!cbrlist.empty())
                cond_constant_propagation();

            v.clear();
            v.push_back(b);
            dead_code_elimination(v, RDS_UNROLL0);

#if defined(DCFG_AFTER)
            dump_cfg(name, "check0", 1);
#endif
        }
        else if (b->get_out(0)->is_mark()) {
            stack.pop_back();
        }
        else if (b->out.size() == 1) {
            stack.push_back(b->get_out(0));
        }
        else if (b->get_out(0)->flags.f_cond_cbranch) {
            stack.push_back(b->get_out(0));
        }
        else if (!b->get_out(0)->get_back_edge_count() && !b->get_out(1)->get_back_edge_count()) {
            stack.push_back(b->get_out(0));
        }
        else {
            bb = bblocks.find_loop_exit(b, get_vmhead());

            blks.clear();
            clone_ifweb(b, b, bb, blks);
            structure_reset();

            blks.clear();
            collect_blocks_to_node(blks, b, bb);
            c = combine_multi_in_before_loop(blks, bb);
            heritage_clear();
            heritage();

#if defined(DCFG_AFTER)
            dump_cfg(name, "check3", 1);
#endif

            stack.push_back(c);

            continue;
        }
    }

    bblocks.clear_marks();

    return true;
}

flowblock*       funcdata::loop_unrolling(flowblock *h, flowblock *end, uint32_t flags, int &meet_exit)
{
    int i, inslot, ret;
    flowblock *start,  *cur, *prev, *br, *tmpb, *exit = NULL;
    list<pcodeop *>::const_iterator it;
    const SeqNum sq;
    pcodeop *p, *op;
    varnode *iv = NULL;

    meet_exit = 0;

    printf("\n\nloop_unrolling sub_%llx \n", h->get_start().getOffset());

    /* 取loop的进入节点*/
    prev = start = loop_pre_get(h, 0);
    /* FIXME:压入trace堆栈 
    这种压入方式有问题，无法识别 undefined bcond
    */
    trace_push(start->last_op());
    cur = h;

    if (flags & _NOTE_VMBYTEINDEX) {
        iv = detect_induct_variable(h, exit);
    }

    do {
        printf("\tprocess flowblock sub_%llx\n", cur->get_start().getOffset());

        it = cur->ops.begin();
        inslot = cur->get_inslot(prev);
        assert(inslot >= 0);

        for (; it != cur->ops.end(); it++) {
            p = *it;

            br = NULL;
            p->set_trace();
            ret = p->compute(inslot, &br);

            if (flags & _DUMP_PCODE) {
                char buf[256];
                p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
                printf("%s\n", buf);
            }

            trace_push(p);
        }

        if ((cur->out.size() > 1) && (ret != ERR_MEET_CALC_BRANCH)) {
            printf("found undefined-bcond in block[%x]\n", cur->sub_id());
            break;
        }

        prev = cur;
        cur = br;

        /* 循环展开到最后一个终止节点 */
        if (exit && (exit == cur)) {
            meet_exit = 1;
            break;
        }
    } while (cur != end);

    /* 把刚才走过的路径复制出来，剔除jmp节点，最后一个节点的jmp保留 */
    br = trace.back()->parent;
    cur = bblocks.new_block_basic();

    if (flags & _NOTE_VMBYTEINDEX) {
        if (!iv->is_constant())
            throw LowlevelError("vm  byteindex must be constant");
        cur->vm_byteindex = iv->get_val();
        printf("def op = %d, val = %d, opcode = %s\n", iv->def->start.getTime(), cur->vm_byteindex, get_opname(iv->def->opcode));
    }

    user_offset += user_step;
    Address addr(d->get_code_space(), user_offset);
    /* 进入节点抛弃 */
    for (i = 0; trace[i]->parent == start; i++);
    /* 从主循环开始 */
    for (; i < trace.size(); i++) {
        funcdata *callfd = NULL;
        p = trace[i];

        if ((p->opcode == CPUI_CALLIND) && p->get_in(0)->is_constant()) {
            Address addr(d->get_code_space(), p->get_in(0)->get_val());
            callfd = d->find_func(addr);
        }

        /* 最后一个节点的jmp指令不删除 */
        if (((i != (trace.size() - 1)) 
            && ((p->opcode == CPUI_BRANCH) || (p->opcode == CPUI_CBRANCH) || (p->opcode == CPUI_INDIRECT) || (p->opcode == CPUI_MULTIEQUAL) || (p->opcode == CPUI_BRANCHIND))))
            continue;

        /* 假如循环展开结束，则最后一个节点不处理 */
        if ((i == (trace.size() - 1)) && meet_exit && (p->opcode == CPUI_CBRANCH)) {
            continue;
        }

        Address addr2(d->get_code_space(), user_offset + p->get_addr().getOffset());
        const SeqNum sq(addr2, op_uniqid++);
        op = cloneop(p, sq);
        op_insert(op, cur, cur->ops.end());

        /* 假如trace以后，发现某个函数可以被计算，把他加入trace列表 */
        if (callfd && !op->callfd) {
            add_callspec(op, callfd);
        }
    }

    for (i = 0; i < trace.size(); i++) {
        pcodeop *p = trace[i];
        p->clear_trace();
        p->compute(-1, &tmpb);
    }

    cur->set_initial_range(addr, addr);
    trace_clear();

    vector<flowblock *> cloneblks;
    /* 到达终点条件有2种 

    1. 一种是碰见了不可计算的cbranch
    2. 一种是碰见了终止节点，比如循环展开时，碰到了头节点
    */
    if (ret != ERR_MEET_CALC_BRANCH) {
        /* 是否要clone节点到终止节点为止？ */
        if (flags & _DONT_CLONE) {
            flowblock *out = br->get_out(0);
            bblocks.add_edge(cur, out, br->out[0].label);
            clear_block_phi(out);

            out = br->get_out(1);
            bblocks.add_edge(cur, out, br->out[1].label);
            clear_block_phi(out);
        }
        else 
            clone_ifweb(cur, br, end, cloneblks);
    }
    else {
        bblocks.add_edge(cur, meet_exit?exit:end);
    }
    cloneblks.push_back(cur);

    /* 删除start节点和loop 头节点的边，连接 start->cur->loop_header */
    int lab = bblocks.remove_edge(start, h);
    bblocks.add_edge(start, cur, lab & a_true_edge);

    clear_block_phi(h);
    clear_block_phi(end);

    remove_unreachable_blocks(true, true);

    structure_reset();

    heritage_clear();
    heritage();

    dead_code_elimination(cloneblks, RDS_UNROLL0);

    return cur;
}


flowblock*  funcdata::clone_block(flowblock *f, u4 flags)
{
    list<pcodeop *>::iterator it;
    flowblock *b;
    pcodeop *op, *p;

    b = bblocks.new_block_basic();
    user_offset += user_step;

    Address addr(d->trans->getDefaultCodeSpace(), user_offset);

    for (it = f->ops.begin(); it != f->ops.end(); it++) {
        op = *it;

        if (op->opcode == CPUI_MULTIEQUAL) continue;
        if ((flags & F_OMIT_RETURN) && (op->opcode == CPUI_RETURN)) break;

        Address addr2(d->get_code_space(), op->get_addr().getOffset());
        SeqNum seq(addr2, op_uniqid++);
        p = cloneop(op, seq);

        op_insert(p, b, b->ops.end());
    }

    b->set_initial_range(addr, addr);

    return b;
}

char*       funcdata::get_dir(char *buf)
{
    funcdata *p = caller;

    while (p && p->caller) p = p->caller;

    if (p)
        sprintf(buf, "%s/%s", d->filename.c_str(), p->name.c_str());
    else 
        sprintf(buf, "%s/%s", d->filename.c_str(), name.c_str());

    return buf;
}

void        funcdata::alias_clear(void)
{
    list<pcodeop *> w = storelist;
    list<pcodeop *>::iterator it;

    w.insert(w.end(), loadlist.begin(), loadlist.end());
    varnode *vn;
    /* 清除别名信息 */
    for (it = w.begin(); it != w.end(); it++) {
        pcodeop *op = *it;

#if 1
        if ((op->opcode != CPUI_STORE) && (op->opcode != CPUI_LOAD)) continue;

        vn = (op->opcode == CPUI_STORE) ? op->output : op->get_virtualnode();

        if (vn)
            destroy_varnode(vn);

        if ((op->opcode == CPUI_LOAD) && (op->num_input() == 3) && !op->get_in(2))
            op->remove_input(2);
#else
        if (op->opcode != CPUI_LOAD) continue;
        if (vn = op->get_virtualnode())
            destroy_varnode(vn);
#endif
    }

    safe_aliaslist.clear();
}

bool        funcdata::have_side_effect(pcodeop *op, varnode *pos)
{
    funcdata *fd = op->callfd;

    if (!op->is_call()) return false;

    dobc *d = op->parent->fd->d;

#if 1
    if (in_safezone(pos->get_val(), pos->size) && (!d->test_cond_inline || !d->test_cond_inline(d, op->get_call_offset())))
        return false;

    return true;
#else
    return fd->have_side_effect();
#endif
}

flowblock*  funcdata::dowhile2ifwhile(vector<flowblock *> &dowhile)
{
    flowblock *b, *dw, *before, *after;
    list<pcodeop *>::iterator  it;

    assert(dowhile.size() == 1);
    
    dw = dowhile[0];

    before = (dw->get_in(0) == dw) ? dw->get_in(1):dw->get_in(0);
    after = (dw->get_out(0) == dw) ? dw->get_out(1) : dw->get_out(0);

    b = clone_block(dowhile[0], 0);

    int label = bblocks.remove_edge(before, dw);

    clear_block_phi(after);
    clear_block_phi(dw);

    while ((after->in.size() == 1) && (after->out.size() == 1)) after = after->get_out(0);

    if (after->in.size() > 1) clear_block_phi(after);

    bblocks.add_edge(before, b, label & a_true_edge);
    bblocks.add_block_if(b, dw, after);
    structure_reset();

    return b;
}

char*       funcdata::print_indent(void)
{
    static char buf[128];
    int i = 0;

    pcodeop *c = callop;

    while (c) {
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';

        c = c->parent->fd->callop;
    }
    buf[i] = 0;

    return buf;
}

bool        funcdata::test_strict_alias(pcodeop *load, pcodeop *store)
{
    if (load->parent != store->parent) {
        return false;
    }

    list<pcodeop *>::iterator it = store->basiciter;

    for (; it != load->basiciter; it++) {
        pcodeop *p = *it;

        if (p->opcode != CPUI_STORE) continue;

        /* 发现store - load链上有一个的地址无法识别，直接停止 */
        if (p->get_in(1)->type.height == a_top)
            return false;
    }

    return true;
}

flowblock*  funcdata::clone_ifweb(flowblock *newstart, flowblock *start, flowblock *end, vector<flowblock *> &cloneblks)
{
    int i, j;
    blockbasic *b, *out;
    vector<flowblock *> stack;
    vector<flowblock *> webs;

    if (start->out.size() != 2)
        throw LowlevelError("only clone ifweb");

    stack.push_back(start->get_out(0));
    stack.push_back(start->get_out(1));
    stack[0]->set_mark();
    stack[1]->set_mark();

    while (!stack.empty()) {
        b = stack.back();
        stack.pop_back();
        webs.push_back(b);

        for (i = 0; i < b->out.size(); i++) {
            out = b->get_out(i);
            if (out == end) continue;

            if (!out->is_mark()) {
                stack.push_back(out);
                out->set_mark();
            }
        }
    }

    for (i = 0; i < webs.size(); i++) {
        b = clone_block(webs[i], 0);
        webs[i]->copymap = b;

        cloneblks.push_back(b);
    }

    if (start == newstart) {
        clear_block_df_phi(start);

        out = start->get_out(0);
        int lab = bblocks.remove_edge(start, out);
        bblocks.add_edge(newstart, out->copymap, lab);

        out = start->get_out(0);
        lab = bblocks.remove_edge(start, out);
        bblocks.add_edge(newstart, out->copymap, lab);
    }
    else {
        out = start->get_out(0);
        bblocks.add_edge(newstart, out->copymap, start->out[0].label);
        out = start->get_out(1);
        bblocks.add_edge(newstart, out->copymap, start->out[1].label);
    }

    for (i = 0; i < webs.size(); i++) {
        for (j = 0; j < webs[i]->out.size(); j++) {
            out = webs[i]->get_out(j);

            bblocks.add_edge(webs[i]->copymap, (out == end) ? out:out->copymap, webs[i]->out[j].label);
        }
    }

    b = webs[0]->copymap;

    for (i = 0; i < webs.size(); i++) {
        webs[i]->copymap = NULL;
        webs[i]->clear_mark();
    }

    return b;
}

flowblock*  funcdata::clone_web(flowblock *start, flowblock *end, vector<flowblock *> &cloneblks)
{
    int i, j;
    blockbasic *b, *out;
    vector<flowblock *> stack;
    vector<flowblock *> webs;

    stack.push_back(start);

    while (!stack.empty()) {
        b = stack.back();
        stack.pop_back();
        b->set_mark();
        webs.push_back(b);

        for (i = 0; i < b->out.size(); i++) {
            out = b->get_out(i);
            if (out == end) continue;

            if (!out->is_mark()) {
                stack.push_back(out);
            }
        }
    }

    for (i = 0; i < webs.size(); i++) {
        b = clone_block(webs[i], 0);
        webs[i]->copymap = b;

        cloneblks.push_back(b);
    }

    for (i = 0; i < webs.size(); i++) {
        for (j = 0; j < webs[i]->out.size(); j++) {
            blockedge *e = &webs[i]->out[j];
            out = e->point;

            bblocks.add_edge(webs[i]->copymap, (out == end) ? out:out->copymap, e->label & a_true_edge);
        }
    }

    b = webs[0]->copymap;

    for (i = 0; i < webs.size(); i++) {
        webs[i]->copymap = NULL;
        webs[i]->clear_mark();
    }

    return b;
}

flowblock *funcdata::inline_call(pcodeop *callop, funcdata *fd)
{
	flowblock *start = callop->parent;
	flowblock *tailb = split_block(start, callop->basiciter);
	flowblock *b, *old, *out;
	int i, j;

	while (start->out.size() > 0) {
		out = start->get_out(0);
		int lab = bblocks.remove_edge(start, out);
		bblocks.add_edge(tailb, out, lab);
	}

	for (i = 0; i < fd->bblocks.get_size(); i++) {
		old = fd->bblocks.get_block(i);
		b = clone_block(old, F_OMIT_RETURN);
		old->copymap = b;
	}
	
	for (i = 0; i < fd->bblocks.get_size(); i++) {
		old = fd->bblocks.get_block(i);
		for (j = 0; j < old->out.size(); j++) {
			blockedge *e = &old->out[j];
			bblocks.add_edge(old->copymap, e->point->copymap, e->label & a_true_edge);
		}
	}

	for (i = 0; i < fd->bblocks.exitlist.size(); i++) {
		old = fd->bblocks.exitlist[i];
		if (old->get_return_addr() == (callop->get_dis_addr() + 4))
			bblocks.add_edge(old->copymap, tailb);
	}
	bblocks.add_edge(start, fd->bblocks.get_block(0)->copymap);

	for (i = 0; i < fd->bblocks.get_size(); i++) {
		old = fd->bblocks.get_block(i);
		old->copymap = NULL;
	}

	return start;
}

func_call_specs::func_call_specs(pcodeop *o, funcdata *f)
{
    op = o;
    fd = f;
}

func_call_specs::~func_call_specs()
{
}

void priority_queue::reset(int maxdepth)
{
    if ((curdepth == -1) && (maxdepth == queue.size() - 1))
        return;

    queue.clear();
    queue.resize(maxdepth + 1);
    curdepth = -1;
}

void priority_queue::insert(flowblock *b, int depth)
{
    queue[depth].push_back(b);
    if (depth > curdepth)
        curdepth = depth;
}

flowblock *priority_queue::extract()
{
    flowblock *res = queue[curdepth].back();
    queue[curdepth].pop_back();
    while (queue[curdepth].empty()) {
        curdepth -= 1;
        if (curdepth < 0)
            break;
    }

    return res;
}

int        dobc::reg2i(const Address &addr)
{
    if (get_addr("r0") <= addr && addr <= get_addr("pc"))
        return (addr.getOffset()  - get_addr("r0").getOffset()) / 4;
    if (get_addr("NG") <= addr && addr <= get_addr("OV"))
        return (addr.getOffset() - get_addr("NG").getOffset()) + 16;

    return -1;
}

int         dobc::vreg2i(const Address &addr)
{
    if (get_addr("s0") <= addr && addr <= get_addr("s31"))
        return (addr.getOffset()  - get_addr("s0").getOffset()) / 4;

    throw LowlevelError("not support");
    return -1;
}

Address     dobc::i2reg(int i)
{
    if (i < 16)
        return get_addr("r0") + i * 4;
    if (i < 20)
        return get_addr("NG") + i;

    throw LowlevelError("not support i");
}

void        dobc::get_scratch_regs(vector<int> &regs)
{
    regs.push_back(R0);
    regs.push_back(R1);
    regs.push_back(R2);
    regs.push_back(R3);
}

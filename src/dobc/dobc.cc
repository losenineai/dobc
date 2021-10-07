

//#include "sleigh_arch.hh"
#include "vm.h"
#include "sleigh.hh"
#include "dobc.hh"
#include "thumb_gen.hh"
#include <iostream>
#include <assert.h>

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

int print_udchain(char *buf, pcodeop *op, uint32_t flags)
{
    varnode *out = op->output;
    int i = 0, j, defs = 0, uses_limit = 10000, defs_limit = 10000;

    if (flags & PCODE_OMIT_MORE_USE)
        uses_limit = 7;

    if (out && out->uses.size()) {
        list<pcodeop *>::iterator iter = out->uses.begin();

        pcodeop  *puse = NULL;

        i += sprintf(buf + i, " [u:");
        for (j = 0; iter != out->uses.end(); iter++, j++) {
            /* 最多打印limit个use */
            if (!uses_limit) break;

            if ((*iter) != puse) {
                i += sprintf(buf + i, "%d ", (*iter)->start.getTime());
                puse = *iter;
                uses_limit--;
            }
        }
        if (!uses_limit)
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

        pcodeop  *pdef = NULL;

        i += sprintf(buf + i, " [d:");
        for (j = 0; j < op->inrefs.size(); j++) {
            if (!defs_limit) break;

            pcodeop *def = op->inrefs[j]->def;
            if (def && (pdef != def)) {
                i += sprintf(buf + i, "%d ", op->inrefs[j]->def->start.getTime());
                pdef = def;
                defs_limit--;
            }
        }

        if (!defs_limit)
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
        else {
            intb x = addr.getOffset();
            if (data->is_sp_vn())
                x -= STACK_BASE;

            if (x > 0)
                return sprintf(buf, "(%c%llx.%d:%d)", addr.getSpace()->getShortcut(), x, data->version, data->size);
            else
                return sprintf(buf, "(%c-%llx.%d:%d)", addr.getSpace()->getShortcut(), abs(x), data->version, data->size);
        }
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

    vector<pcodeop_lite *> &pvec(fd->litemap[addr]);

    pvec.push_back(fd->cloneop_lite(op));

#if 0
    if (d->is_adr(addr)) {
        vn->set_pc_constant(vn->get_offset());
        vn->flags.from_pc = 1;
    }
#endif

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

void        dobc::add_func(funcdata *fd)
{
    if (fd->flags.thumb) {
        addrtab[fd->startaddr + 1] = fd;
    }

    addrtab[fd->startaddr] = fd;
    nametab[fd->name] = fd;
}

funcdata*   dobc::add_func(const string &name)
{
    funcdata *fd = NULL;

    if ((fd = find_func(name))) return fd;

    LoadImageSymbol *sym = loader->getSymbol(name);
    if (!sym)
        return NULL;

    fd = new funcdata(name.c_str(), sym->address, 0, this);

    add_func(fd);

    return fd;
}

funcdata*   dobc::add_func(const Address &a)
{
    char buf[128];
    funcdata *fd = NULL;
    if ((fd = find_func(a))) return fd;

    if (a.getSpace() == ram_spc) {
        Address addr(getDefaultCodeSpace(), a.getOffset());

        sprintf(buf, "sub_%llx", a.getOffset());
        fd = new funcdata(buf, addr, 0, this);
    }
    else if (a.getSpace() == getDefaultCodeSpace()) {
        sprintf(buf, "sub_%llx", a.getOffset());
        fd = new funcdata(buf, a, 0, this);
    }

    add_func(fd);

    return fd;
}

void dobc::init_plt()
{
    funcdata *fd;
    int i;

    if (shelltype == SHELL_360FREE) {
        for (i = 0; i < count_of_array(pltlist); i++) {
            struct pltentry *entry = &pltlist[i];

            Address addr(getDefaultCodeSpace(), entry->addr);
            fd = new funcdata(entry->name, addr, 0, this);
            fd->set_noreturn(entry->exit);
            fd->funcp.set_side_effect(entry->side_effect);
            fd->funcp.inputs = entry->input;
            fd->funcp.output = 1;
            addrtab[addr] = fd;
        }
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

void dobc::add_space_base(AddrSpace *basespace, 
    const string &nm, const VarnodeData &ptrdata, int trunsize, bool isreversejustified, bool stackGrowth)
{
    int ind = numSpaces();

    SpacebaseSpace *spc = new SpacebaseSpace(this, trans, nm, ind, trunsize, basespace, ptrdata.space->getDelay()+1);

    insertSpace(spc);
    addSpacebasePointer(spc, ptrdata, trunsize, stackGrowth);
}

funcdata* test_vmp360_cond_inline(dobc *d, intb addr)
{
    int i;

    for (i = 0; i < count_of_array(pltlist); i++) {
        pltentry *e = pltlist + i;
        if ((e->addr == addr) && strstr(e->name, "vmp360")) {
            uintb uaddr = (uintb)addr;
            Address addr(d->getDefaultCodeSpace(), uaddr);
            return d->find_func(addr);
        }
    }

    return NULL;
}

void dobc::plugin_ollvm()
{
    funcdata *fd;

    for (int i = 0; i < decode_symbol_list.size(); i++) {
        if ((fd = add_func(decode_symbol_list[i])))
            fd->ollvm_deshell();
    }

    for (int i = 0; i < decode_address_list.size(); i++) {
        if ((fd = add_func(Address(trans->getDefaultCodeSpace(), decode_address_list[i]))))
            fd->ollvm_deshell();
    }

    loader->saveFile(out_dir + out_filename);
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
    for (int i = 0; i < numSpaces(); i++) {
        AddrSpace *spc = getSpace(i);
        if (spc->getName() == "ram")
            ram_spc = spc;
        else if (spc->getName() == "register")
            reg_spc = spc;

        print_info("Space[%s, type:%d, size:%llx, shortcut:%c\n", spc->getName().c_str(), spc->getType(), spc->getHighest(), spc->getShortcut());
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
        if (name.find("mult") != string::npos) continue;
        if (name == "TB") continue;
        if (name == "ISAModeSwitch") continue;
        cpu_regs.insert(it->first.getAddr());
    }
}

void dobc::gen_sh(void)
{
    char buf[260];

    sprintf(buf, "%s/gen.sh", filename.c_str());
    file_save(buf, GEN_SH, strlen(GEN_SH));
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

pcodeop*    flowblock::first_callop(void)
{
    list<pcodeop *>::iterator it;

    for (it = ops.begin(); it != ops.end(); it++) {
        pcodeop *p = *it;

        if (p->callfd) return p;
    }

    return NULL;
}

/* 基于pcode结构做的比较

1. opcode必须一致
2. phi节点例外
3. 任何 非uniq 变量必须一致
4. const部分必须一致
5. 所有output和in的 size必须一致
*/
int pcodeop_struct_cmp(pcodeop *a, pcodeop *b, uint32_t flags)
{
    int i;

    if (!a || !b) return -1;

    dobc *d = a->parent->fd->d;

    if (!flags && (a->get_addr() != b->get_addr())) return -1;
    if (a->opcode == CPUI_MULTIEQUAL) return -1;
    if (a->opcode != b->opcode) return -1;
    if (a->output) {
        if ((d->is_temp(poa(a)) != d->is_temp(poa(b))) || (!d->is_temp(poa(a)) && (poa(a) != poa(b)))) return -1;
    }

    /* 不比较load的虚拟节点 */
    int sz = a->inrefs.size();
    if (a->opcode == CPUI_LOAD) sz = 2;

    for (i = 0; i < sz; i++) {
        varnode *l = a->get_in(i);
        varnode *r = b->get_in(i);
        bool istemp = d->is_temp(l->get_addr());
        if ((istemp == d->is_temp(r->get_addr()))) continue;
        if (!istemp && (l->get_addr() == r->get_addr()) && (l->get_size() == r->get_size())) continue;

        return -1;
    }

    return 0;
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
                    throw LowlevelError("dont remove store have use");
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
    searchvn(this, 0, Address(Address::m_minimal)),
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
    varnode *vn = new varnode(this, s, m);

    vn->create_index = vbank.create_index++;
    vn->lociter = loc_tree.insert(vn).first;
    vn->defiter = def_tree.insert(vn).first;
    return vn;
}

varnode*    funcdata::create_def(int s, const Address &m, pcodeop *op)
{
    varnode *vn = new varnode(this, s, m);
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
    Address addr(d->getConstantSpace(), val);

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
        searchvn.loc = Address(d->getNextSpaceInOrder(space), 0);
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
    searchvn.loc = Address(d->getNextSpaceInOrder(spaceid), 0);
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

    list<pcodeop *>::iterator it = p->insertiter;
    pcodeop *prev = *--it;

    /*
    blx指令在转换成pcode时，会携带一个 TB标记，比如:

        blx 0x1380

    p748 [  0]:(%lr.3:4) = COPY (#3669:4)          3669 [u:751]
    p749 [  1]:(%ISAModeSwitch.3:1) = COPY (#0:1)  0 [u:750]
    p750 [  2]:(%TB.3:1) = COPY (%ISAModeSwitch.3:1) 0 [d:749]
    p751 [  3]:(%r0.21:4) = CALL.__stack_check_fail 

    这个标记一般在call指令的前一条
    */
    if ((prev->get_addr() == p->get_addr()) && (prev->opcode == CPUI_COPY) && (poa(prev) == d->get_addr("TB"))) {
        vn = new_varnode(1, poa(prev));
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
    int noreturn = 0;

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
                Address destaddr(d->getConstantSpace(), op->get_in(0)->get_val());
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
                    if (noreturn = fd->flags.noreturn) startbasic = true;

                    add_callspec(op, fd);
                }
                op->flags.exit = noreturn;
        }
        break;

        case CPUI_CALLIND:
			add_callspec(op, NULL);
            break;
        }

        if ((op->opcode == CPUI_BRANCH
            || op->opcode == CPUI_BRANCHIND
            || op->opcode == CPUI_RETURN
            || noreturn) && (op->start.getTime() >= maxtime)) {
            del_remaining_ops(oiter);
            oiter = deadlist.end();
        }
    }

    if (noreturn)
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


    try {
        if (flags.dump_inst) 
        {
            d->trans->printAssembly(assem, curaddr);
        }

        assem.set_mnem(1);
        d->trans->printAssembly(assem, curaddr);
    }
    /* 这一个地方是无法避免的， 我们看以下代码:
    
    0x01040: bl r3
    0x01042: data0 data1

    当我们走到 1040 时，因为我们无法成功计算r3的地址，所以我们要继续往下，但是r3其实是一个noreturn函数，
    我们继续fallthru时，结果找不到数据对应的指令直接就崩溃了。我们需要把崩溃的前一条pcode打上exit的标记，
    这样在split_basic时，会在这里停止
    */
    catch (LowlevelError &err) {
        printf("Instructon not implement 0x%llx\n", curaddr.getOffset());
        oiter = --deadlist.end();
        (*oiter)->flags.exit = 1;
        return false;
    }
    
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
            if (op->get_in(0)->is_constant() || op->get_in(0)->is_pc_constant()) {
                Address addr(d->getDefaultCodeSpace(), op->get_in(0)->get_val());

                target_op = find_op(addr);
                if (target_op)
                    block_edge.push_back(new op_edge(op, target_op));
                else
                    printf("not found address[%llx]  \n", addr.getOffset());
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
                list<pcodeop *>::const_iterator it = iter, next_it, it1;
                pcodeop *p;

combine_it_label:
                /*
.text:0003E712 368 01 BF       ITTTT EQ
.text:0003E714 368 24 99       LDREQ           R1, [SP,#0x360+var_2D0]
.text:0003E716 368 08 68       LDREQ           R0, [R1]
.text:0003E718 368 00 BA       REVEQ           R0, R0

合并IT block种的指令为一个block
                */
                for (; (it != deadlist.end()) && (*it)->flags.itblock; it = next_it) {
                    p = *it;
                    next_it = ++it;

                    if (p->opcode == CPUI_CBRANCH) {
                        if (cmp_itblock_cbranch_conditions(op, p)) {
                            break;
                        }
                        else {
                            op_unset_input(op, 0);
                            op_set_input(op, clone_varnode(p->get_in(0)), 0);
                            op_destroy_raw(p);
                        }
                    }
                }

                /* 来自于 libkwsgmain.so[bf8035a0f4c9680a9b53eb225bbe12fd]
                有这样一种情况 

.text:0003E712 368 01 BF       ITTTT EQ
.text:0003E714 368 24 99       LDREQ           R1, [SP,#0x360+var_2D0]
.text:0003E716 368 08 68       LDREQ           R0, [R1]
.text:0003E718 368 00 BA       REVEQ           R0, R0
.text:0003E71A 368 41 F8 04 0B STREQ.W         R0, [R1],#4
.text:0003E71E 368 02 BF       ITTT EQ
.text:0003E720 368 2A 91       STREQ           R1, [SP,#0x360+var_2B8]
.text:0003E722 368 4C F6 2F 51 MOVWEQ          R1, #0xCD2F
.text:0003E726 368 CB F6 A3 41 MOVTEQ          R1, #0xBCA3

我们把3e712开始的it块指令，全部粘合在了一起，结果发现下面的3E71E它也是一个it块，而这个it块的条件和上面一个
IT块是完全一样的，这个时候我们需要把3E720 - 3E726的代码，和上面的 3E714-3E71A的指令粘合在一起
                */

                p = NULL;
                if (it != deadlist.end()) it++;
                if (it != deadlist.end()) p = *it;

                if (p && p->flags.itblock) {
                    while (p->opcode != CPUI_CBRANCH) p = *++it;

                    if (0 == cmp_itblock_cbranch_conditions(op, p))
                        goto combine_it_label;
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

void        funcdata::op_insert(pcodeop *op, flowblock *bl, list<pcodeop *>::iterator iter)
{
    mark_alive(op);
    bl->insert(iter, op);
}

void        funcdata::op_insert_begin(pcodeop *op, flowblock *bl)
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

void        funcdata::op_insert_end(pcodeop *op, flowblock *bl)
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

        print_detail("0x%x -> 0x%x\n", (int)edge->from->start.getAddr().getOffset(), (int)edge->to->start.getAddr().getOffset());
    }
}

void        funcdata::split_basic()
{
    pcodeop *op;
    flowblock *cur;
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
            flowblock *newfront = bblocks.new_block_basic();
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
    char buf[64 * 1024];
    Address prev_addr;
    AssemblyRaw assememit;
    pcodeop *p;

    sprintf(buf, "%s/pcode_%s.txt", get_dir(buf), postfix);
    fp = fopen(buf, "w");

    list<pcodeop *>::iterator iter = deadlist.begin();

    assememit.set_fp(fp);

    for (int i = 0; i < d->numSpaces(); i++) {
        AddrSpace *spc = d->getSpace(i);
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

void        funcdata::dump_block(FILE *fp, flowblock *b, int flag)
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
            "<tr><td><font color=\"green\">sub_%llx(%d,%d, h:%d, vbi:%d, vci:%d, os:%d)</font></td></tr>",
            b->sub_id(),
            block_color(b),
            b->get_start().getOffset(),
            b->dfnum,
            b->index, domdepth.size() ? domdepth[b->index]:0,
            b->vm_byteindex,
            b->vm_caseindex, b->ops.size());
    }
    else {
        fprintf(fp, "loc_%x [style=\"filled\" fillcolor=%s label=<<table bgcolor=\"white\" align=\"left\" border=\"0\">"
                    "<tr><td><font color=\"red\">sub_%llx(df:%d, ind:%d, domh:%d, outl:%d, looph_df:%d, os:%d)</font></td></tr>",
            b->sub_id(),
            block_color(b),
            b->get_start().getOffset(),
            b->dfnum,
            b->index, domdepth.size() ?  domdepth[b->index]:0, b->is_out_loop(), b->loopheader ? b->loopheader->dfnum:0, b->ops.size());
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
    if (NULL == fp) 
        vm_error("fopen failure %s", obuf);

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    int i, j;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        flowblock *b = bblocks.blist[i];

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

void        funcdata::dump_cfg(const string &name, const string &postfix, int dumppcode)
{
    char obuf[512];

    sprintf(obuf, "%s/cfg_%s_%s.dot", get_dir(obuf), name.c_str(), postfix.c_str());

    FILE *fp = fopen(obuf, "w");
    if (NULL == fp) 
        vm_error("fopen failure %s", obuf);

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    int i, j, k;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        flowblock *b = bblocks.blist[i];

        dump_block(fp, b, dumppcode);
    }

    int size = bblocks.blist.size();
    for (i = 0, k = 0; i < size; ++i) {
        flowblock *b = bblocks.blist[i];

        for (j = 0; j < b->out.size(); ++j, k++) {
            blockedge *e = &b->out[j];

            fprintf(fp, "loc_%x ->loc_%x [label = \"%s\" color=\"%s\" penwidth=%d]\n",
                b->sub_id(), e->point->sub_id(),  e->is_true() ? "true":"false", edge_color(e), edge_width(e));
        }
    }

    //dump_rank(fp);

    fprintf(fp, "}");

    fclose(fp);
}

void        funcdata::dump_loop(const char *postfix)
{
    char obuf[512];

    sprintf(obuf, "%s/loop_%s.dot", get_dir(obuf), postfix);

    FILE *fp = fopen(obuf, "w");
    if (NULL == fp) 
        vm_error("fopen failure %s", obuf);

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    int i;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        flowblock *b = bblocks.blist[i];

        dump_block(fp, b, 0);
    }

    flowblock *loop_header;
    for (i = 0; i < bblocks.blist.size(); ++i) {
        flowblock *b = bblocks.blist[i];
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
    if (NULL == fp) 
        vm_error("fopen failure %s", obuf);

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
    Address addr(d->getUniqueSpace(), vbank.uniqid);

    vbank.uniqid += s;

    return create_vn(s, addr);
}

varnode*    funcdata::new_unique_out(int s, pcodeop *op)
{
    varnode* vn = create_def_unique(s, op);

    op->set_output(vn);

    return vn;
}

/*
    对于load, store节点，根据需要生成额外的 mem节点

1.   temp0 = r0 + 4
2.   store ram[temp0] r1
3.   temp1 = r0 + 4
4.   r2 = load ram[temp1]

    temp0和temp1的地址其实是等价的，但是指令2，和指令4，没有关联起来，我们生成一个新的mem节点，把代码重写如下:

1.   temp0 = r0 + 4
2.   store ram[temp0] r1 mem0
3.   temp1 = r0 + 4
4.   r2 = load ram[temp1] mem0

    temp0或者temp1实际上就是取mem0的地址:

    temp0 = &mem0


    但是这里有个问题，是如何设计mem节点的数据结构。传统的Ghidra使用了一种叫StackBaseSpace的方法，来表现大部分的load, store访问，
    我讲下我对这一块的理解（不保证理解的绝对正确性）:

    1. translate.hh:170 的注释，SpaceBaseSpace这个结构的需求是来自于大部分的程序分析需要分析局部变量，
       而局部变量都可以表示成相对于sp的偏移，所以他们设计了一种虚拟space。
    2. 每个SpaceBaseSpace必须关联一个base register

    我这里没搞懂的是，假如我有2个virtual space的需求，生成了2个SpaceBaseSpace，它们的base register都是一样的，在Ghidra的框架设计
    中应该如何区分，比如以下代码:

1.    r0 = malloc(128);
2.    store [r0+4], r1
3.    store [sp-4], r0

4.    r0 = malloc(128);
5.    store [r0 + 4], r2
6.    store [sp-8], r0

    inst 1和4处的r0其实代表不同的虚拟地址空间，但是Ghidra只用了最基本的VarnodeData的数据结构去描述这个baseSpace之间的区别？那面对2个
    一样的baseRegister该如何处理，它们内在的语义已经完全发生了区别.

    搜了下代码，architecture.hh:177, getSpaceBySpaceBase() 是根据loc, size来区分的

    既然Ghidra这里确实只处理了最基本的Stack的虚拟空间的问题，那么我们可能需要重新调整它的设计以适应更多的虚拟地址空间的问题。怎么改？

    我们这里先使用默认的StackBaseSpace，因为我现在也暂时只用到了Stack virtual space. 后面但我们有更多的virtual space时，对整个系统插入
    更多的virtual space来却分，

    每个virtual space可能要关联的是pcode，而不是varnode
*/

varnode*    funcdata::new_mem(AddrSpace *spc, int offset, int s)
{
    Address addr(spc, offset);

    return create_vn(s, addr);
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
        if (!vn->is_sp_vn())
            print_warn("try to remove varnode have def[%d] forbidden", vn->def->start.getTime());
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

    newop1->flags.itblock = op->flags.itblock;
    newop1->flags.startinst = op->flags.startinst;
    newop1->flags.startblock = op->flags.startblock;
	newop1->flags.uncalculated_store = op->flags.uncalculated_store;
    if (op->output)
        op_set_output(newop1, clone_varnode(op->output));
    
    for (i = 0; i < sz; i++)
        op_set_input(newop1, clone_varnode(op->get_in(i)), i);

    newop1->callfd = op->callfd;
    newop1->disaddr = new Address (op->get_dis_addr());

    return newop1;
}

pcodeop_lite*    funcdata::cloneop_lite(pcodeop *op)
{
    int sz = op->inrefs.size();
    pcodeop_lite *lite = new pcodeop_lite(sz);
    varnode *vn;

    lite->opcode = op->opcode;

    if (op->output) {
        vn = new varnode(this, op->output->size, op->output->get_addr());

        lite->output = vn;
    }

    for (int i = 0; i < sz; i++) {
        varnode *vn = new varnode(this, op->get_in(i)->size, op->get_in(i)->get_addr());

        lite->inrefs[i] = vn;
    }

    return lite;
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
    /* 这个删除必须在op_destroy前面，因为 op_destroy会把op从它的parent block list中取出 */
    if ((op->opcode == CPUI_STORE) && op->parent->sideeffect_ops.size())
        op->parent->sideeffect_ops.erase(op->sideiter);

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
    fd.dump_cfg(fd.name, "cond_before", 1);
#endif

    fd.argument_pass();

#if defined(DCFG_COND_INLINE)
    fd.dump_cfg(fd.name, "cond_after", 1);
#endif

    flowblock *b = fd.bblocks.get_block(0);
    list<pcodeop *>::iterator it = b->ops.begin();

    /* 把condline后的首blk的头节点都复制到 callop的后面*/
	if (fd.bblocks.get_size() < 5) {
		for (; it != b->ops.end(); it++) {
			pcodeop *p = *it;
			pcodeop *op;

			if (p->opcode == CPUI_RETURN) break;

			Address addr2(d->getDefaultCodeSpace(), user_offset + p->get_addr().getOffset());
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

void        funcdata::phi_clear()
{
    pcodeop *p;
    for (int i = 0; i < bblocks.get_size(); i++) {
        flowblock *b = bblocks.get_block(i);

        while ((p = b->first_op()) && (p->opcode == CPUI_MULTIEQUAL))
            op_destroy_ssa(p);
    }
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

    Address baddr(d->getDefaultCodeSpace(), 0);
    Address eaddr(d->getDefaultCodeSpace(), ~((uintb)0));
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
            pcodeop *def, *def1;

            def = a->def;

            if (a->type.height == a_top) {
                /* 判断是否在同一个函数内 */
                if (b->fd == this) {
                    def1 = load ? load->get_in(1)->def:NULL;
                    /* 判断是否对同一个space在做操作 

                    r1.5 = T
                    
                    store [r1.5], r2

                    r0 = load r1

                    在这里即使r1的值不可计算，也是可以关联的 
                    */
                    if (!def1 || (def1->get_in(0) != def->get_in(0))) {
                        *maystore = p;
                    }
                    /* 

                    FIXME: 后面要增加stack子空间的算法，这个只是临时处理下，为了应对libmetasec_ml:0x4fc58
                    */
                    else if ((def->opcode == def1->opcode) && def1->get_in(1)->is_constant() && def->get_in(1)->is_constant() 
                        && (def1->get_in(1)->get_val() == def->get_in(1)->get_val())) {
                        return p;
                    }
                }

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
                            *maystore = p;
                            return NULL;
                        }

                        if (a->type == pos->type)
                            return NULL;
                    }
                    else if ((p->opcode == CPUI_COPY) && p->output->is_sp_constant()) {
                        varnode *a = p->output;
                        if (a->type == pos->type)
                            return NULL;
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

bool        funcdata::loop_unrolling4(flowblock *h, int vm_caseindex, uint32_t flags)
{
    int meet_exit, i, inslot;
    flowblock *cur = loop_unrolling(h, h, flags, meet_exit);
    vector<flowblock *> blks;
    pcodeop *p;

    cur->vm_caseindex = vm_caseindex;

    if (meet_exit) return true;

    if (flags & _DUMP_ORIG_CASE)
        return 0;

#if defined(DCFG_BEFORE)
    dump_cfg(name, to_string(vm_caseindex) + "_orig" , 1);
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
        inslot = h->get_in_index(blks[0]);
        iv = detect_induct_variable(h, c);
        vn = iv->def->get_in(inslot);

        if ((blks.size() == 1) && vn->is_constant()) 
            break;

        if ((blks.size() > 1) && vn->is_constant()) {
            for (i = 1; i < blks.size(); i++) {
                inslot = h->get_in_index(blks[i]);
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

    print_tag("\n\nloop_unrolling sub_%llx \n", h->get_start().getOffset());

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
        print_info("\tprocess flowblock sub_%llx\n", cur->get_start().getOffset());

        it = cur->ops.begin();
        inslot = cur->get_in_index(prev);
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
    Address addr(d->getDefaultCodeSpace(), user_offset);
    /* 进入节点抛弃 */
    for (i = 0; trace[i]->parent == start; i++);
    /* 从主循环开始 */
    for (; i < trace.size(); i++) {
        funcdata *callfd = NULL;
        p = trace[i];

        if ((p->opcode == CPUI_CALLIND) && p->get_in(0)->is_constant()) {
            Address addr(d->getDefaultCodeSpace(), p->get_in(0)->get_val());
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

        Address addr2(d->getDefaultCodeSpace(), user_offset + p->get_addr().getOffset());
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

        Address addr2(d->getDefaultCodeSpace(), op->get_addr().getOffset());
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

        if ((op->opcode != CPUI_STORE) && (op->opcode != CPUI_LOAD)) continue;

        vn = (op->opcode == CPUI_STORE) ? op->output : op->get_virtualnode();

        if (vn)
            destroy_varnode(vn);

        if ((op->opcode == CPUI_LOAD) && (op->num_input() == 3) && !op->get_in(2))
            op->remove_input(2);
    }
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

bool        funcdata::is_ifthenfi_structure(flowblock *top, flowblock *middle, flowblock *bottom)
{
    if (!top->is_cbranch()) return false;
    if ((middle->get_0out() == bottom) && (middle->get_0in() == top)) return true;

    return false;
}

bool        funcdata::is_ifthenfi_phi(pcodeop *phi, varnode *&top, varnode *&branch)
{
    if (phi->inrefs.size() != 2 || !phi->all_inrefs_is_adj())
        return false;

    varnode *in0 = phi->get_in(0);
    varnode *in1 = phi->get_in(1);

    flowblock *topb = in0->def->parent;
    flowblock *branchb = in1->def->parent;

    if (topb->dfnum > branchb->dfnum) {
        topb = in1->def->parent;
        branchb = in0->def->parent;
    }

    if ((branchb->out.size() == 1) && (branchb->in.size() == 1) && (branchb->get_in(0) == topb)) {
        top = (topb == in0->def->parent) ? in0 : in1;
        branch = (branchb == in0->def->parent) ? in0 : in1;
        return true;
    }

    return false;
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
    flowblock *b, *out;
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
    flowblock *b, *out;
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

void        dobc::build_loader(DocumentStorage &store)
{
    loader = new ElfLoadImage(fullpath);
    loader1 = new ElfLoadImage(fullpath);
}

void        dobc::build_context()
{
    context = new ContextInternal();
}

Translate*  dobc::build_translator(DocumentStorage &store)
{
    Translate *sleigh = new Sleigh(loader, context);

    return sleigh;
}

void dobc::restore_from_spec(DocumentStorage &store)
{
    Translate *newtrans = build_translator(store);
    newtrans->initialize(store);
    trans = newtrans;
    copySpaces(newtrans);
    trans->getUserOpNames(useroplist);

    parseCompilerConfig(store);

    trans->setContextDefault("TMode", 1);

    loader->setCodeSpace(getDefaultCodeSpace());
    loader->init();
}

void        dobc::build_arm()
{
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
    ng_addr = trans->getRegister("NG").getAddr();
    ov_addr = trans->getRegister("OV").getAddr();
    pc_addr = trans->getRegister("pc").getAddr();

    tzr_addr = trans->getRegister("tmpZR").getAddr();
    tcy_addr = trans->getRegister("tmpCY").getAddr();
    tng_addr = trans->getRegister("tmpNG").getAddr();
    tov_addr = trans->getRegister("tmpOV").getAddr();

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

static string strip__(string &name)
{
    const char *s = name.c_str();

    while (*s && *s == '_') s++;

    return string(s);
}

void dobc::init(DocumentStorage &store)
{
    build_loader(store);
    build_context();
    
    restore_from_spec(store);

    build_arm();
    build_function_type();

    init_spcs();
    init_regs();
    init_abbrev();
    init_plt();
    init_syms();
}

void dobc::init_syms()
{
    addrtab::iterator it;
    LoadImageFunc *sym;
    funcdata *fd;

    for (it = loader->beginSymbol(); it != loader->endSymbol(); it++) {
        sym = it->second;

        if (NULL == (fd  = find_func(sym->address)))
            fd = new funcdata(sym->name.c_str(), sym->address, sym->size, this);

        /* 这里之所以这么做，是因为Ghidra在设置noreturn的列表时，这个默认的列表，是删除了头部的 _ 符号的。*/
        if (noreturn_func_tab.find(strip__(sym->name)) != noreturn_func_tab.end())
            fd->set_noreturn(1);

        add_func(fd);
    }

    for (int i = 0; i < noreturn_calls.size(); i++) {
        Address addr(getDefaultCodeSpace(), noreturn_calls[i]);

        if (NULL == (fd = find_func(addr))) {
            fd = add_func(addr);
        }

        fd->set_noreturn(1);
    }

    zero_addr = Address(getDefaultCodeSpace(), 0);
}

void dobc::parse_stack_pointer(const Element *el)
{
    AddrSpace *basespace = getSpaceByName("ram");
    bool stackGrowth = true;
    bool isreversejustify = false;

    VarnodeData point = trans->getRegister("sp");
    int truncsize = point.size;
    if (basespace->isTruncated() && (point.size > basespace->getAddrSize()))
        truncsize = basespace->getAddrSize();

    add_space_base(basespace, "stack", point,  truncsize, false, stackGrowth);
}

void dobc::parseCompilerConfig(DocumentStorage &store)
{
    parse_stack_pointer(NULL);
}

AddrSpace *dobc::getSpaceBySpacebase(const Address &loc, int4 size) const
{
    AddrSpace *id;
    int4 sz = numSpaces();
    for (int4 i = 0; i < sz; ++i) {
        id = getSpace(i);
        if (id == (AddrSpace *)0) continue;
        int4 numspace = id->numSpacebase();
        for (int4 j = 0; j < numspace; ++j) {
            const VarnodeData &point(id->getSpacebase(j));
            if (point.size != size) continue;
            if (point.space != loc.getSpace()) continue;
            if (point.offset != loc.getOffset()) continue;
            return id;
        }
    }
    throw LowlevelError("Unable to find entry for spacebase register");
    return (AddrSpace *)0;
}

void    dobc::build_noreturn_function()
{
    string noreturn_config = ghidra + "/Features/Base/data/noReturnFunctionConstraints.xml";

    DocumentStorage storage;
    Element *root = storage.openDocument(noreturn_config)->getRoot();

    const List &list(root->getChildren());
    List::const_iterator iter;
    
    for (iter = list.begin(); iter != list.end(); iter++) {
        string name = (*iter)->getAttributeValue("name");

        if (name.find("ELF") != -1) {
            Element *child = *((*iter)->getChildren().begin());
            noreturn_elf = child->getContent();
            break;
        }
    }

    if (noreturn_elf.empty())
        vm_error("noreturn elf file is empty");

    string noreturn_elf_fullpath = ghidra + "/Features/Base/data/" + noreturn_elf;

    ifstream infile(noreturn_elf_fullpath);
    string line;

    while (getline(infile, line)) {
        if (line.rfind("# ", 0) == 0) continue;
        noreturn_func_tab.insert(line);
    }
}

void    dobc::build_function_type()
{
    build_noreturn_function();
}

void    dobc::set_ghidra(const char *ghidra_path)
{
    ghidra.assign(ghidra_path);
    slafilename = ghidra + "/" + ARM_SLA;
}

void    dobc::set_input_bin(const char *bin)
{
    fullpath.assign(bin);
    filename.assign(basename(bin));

    if (out_filename.empty()) {
        out_filename.assign(basename(bin));
        out_filename += ".decode";
    }

    mdir_make(filename.c_str());
    gen_sh();
}

dobc::dobc() 
{
    g_dobc = this;
}

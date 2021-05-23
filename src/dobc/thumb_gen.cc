
#include "sleigh.hh"
#include "thumb_gen.hh"
#include "pass.hh"
#include <assert.h>
#include "vm.h"

#define a(vn)               ((vn)->get_addr())
#define ama                 d->ma_addr
#define asp                 d->sp_addr
#define ar0                 d->r0_addr
#define apc                 d->pc_addr
#define alr                 d->lr_addr
#define azr                 d->zr_addr
#define ang                 as("NG")
#define aov                 as("OV")
#define acy                 as("CY")
#define istemp(vn)          ((vn)->get_addr().getSpace()->getType() == IPTR_INTERNAL)
#define isreg(vn)           d->is_greg(vn->get_addr())
#define as(st)              d->trans->getRegister(st).getAddr()

#define ANDNEQ(r1, r2)      ((r1 & ~r2) == 0)
#define ANDEQ(r1, r2)      ((r1 & r2) == r2)
#define in_imm3(a)          ANDNEQ(a, 0x07)
#define in_imm4(a)          ANDNEQ(a, 0x0f)
#define in_imm5(a)          ANDNEQ(a, 0x1f)
#define in_imm7(a)          ANDNEQ(a, 0x7f)
#define in_imm8(a)          ANDNEQ(a, 0xff)
#define align4(a)           ((a & 3) == 0)

#define COND_EQ         0
#define COND_NE         1
#define COND_CS         2
#define COND_CC         3
#define COND_MI         4
#define COND_PL         5
#define COND_VS         6
#define COND_VC         7
#define COND_HI         8
#define COND_LS         9
#define COND_GE         10
#define COND_LT         11
#define COND_GT         12
#define COND_LE         13
#define COND_AL         14

/* A8.8.119 */
#define NOP1            0xbf00
#define NOP2            0xf3af8000

#define imm_map(imm, l, bw, r)          (((imm >> l) & ((1 <<  bw) - 1)) << r)
#define bit_get(imm, l, bw)             ((imm >> l) & ((1 << bw) - 1))

#define read_thumb2(b)          ((((uint32_t)(b)[0]) << 16) | (((uint32_t)(b)[1]) << 24) | ((uint32_t)(b)[2]) | (((uint32_t)(b)[3]) << 8))
#define write_thumb2(i, buf) do { \
            (buf)[2] = i & 255; \
            i >>= 8; \
            (buf)[3] = i & 255; \
            i >>= 8; \
            (buf)[0] = i & 255; \
            i >>= 8; \
            (buf)[1] = i & 255; \
            i >>= 8; \
    } while (0)

uint32_t bitcount(uint32_t x)
{
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
    x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
    x = (x & 0x0000FFFF) + ((x >>16) & 0x0000FFFF);

    return x;
}

int nlz(uint32_t x) 
{
    int n;
    if (x == 0) return(32); n = 1;
    if ((x >> 16) == 0) { n = n + 16; x = x << 16; }
    if ((x >> 24) == 0) { n = n + 8; x = x << 8; }
    if ((x >> 28) == 0) { n = n + 4; x = x << 4; }
    if ((x >> 30) == 0) { n = n + 2; x = x << 2; }
    n = n - (x >> 31);
    return n;
}

int ntz(unsigned x) 
{
    int n;
    if (x == 0) return(32);
    n = 1;
    if ((x & 0x0000FFFF) == 0) { n = n + 16; x = x >> 16; }
    if ((x & 0x000000FF) == 0) { n = n + 8; x = x >> 8; }
    if ((x & 0x0000000F) == 0) { n = n + 4; x = x >> 4; }
    if ((x & 0x00000003) == 0) { n = n + 2; x = x >> 2; }
    return n - (x & 1);
}

int continuous_zero_bits(uint32_t x, int *lsb, int *width)
{
    /* 检查x是否只有一段0的格式 */

    /* 去掉低部的连续1，兼容以0开始 */
    uint32_t t = ~((x + 1) & x);
    if ((t + 1) & t)
        return -1;

    if (x == (uint32_t)-1) return -1;

    int i = 0;

    while (x & (1 << i)) i++;

    *lsb = i;
    *width = ntz((x >> i));

    return 0;
}


/* 测试x里是否有连续的1 */
#define bitcont(x)          (!(((~x + 1) & x + x) & x))

static  thumb_gen *g_cg = NULL;

thumb_gen::thumb_gen(funcdata *f) : codegen(f)
{
    fd = f;
    d = fd->d;
    g_cg = this;

    data = d->loader->filedata;
    ind = fd->get_addr().getOffset();

    int size = fd->get_size();

    maxend = size + ind;
    end = size + ind;

    memset(data + ind, 0, size);
}

thumb_gen::~thumb_gen()
{
}

void thumb_gen::resort_blocks()
{
    blist = fd->bblocks.blist;
}

#define UNPREDICITABLE()        vm_error("not support");

uint32_t thumb_gen::reg2i(const Address &a)
{
    return d->reg2i(a);
}

bool thumb_gen::simd_need_fix(pit it)
{
    return false;
}


static void ot(uint16_t i)
{
    g_cg->data[g_cg->ind++] = i & 255;
    i >>= 8;
    g_cg->data[g_cg->ind++] = i & 255;
}

static void ott(uint32_t i)
{
    write_thumb2(i, g_cg->data + g_cg->ind);
    g_cg->ind += 4;
}

static void ob(uint8_t *buf, int siz)
{
    memcpy(g_cg->data + g_cg->ind, buf, siz);
    g_cg->ind += siz;
}

static void o(uint32_t i)
{
    if (i == 0)
        vm_error("meet 0's value instruction");

    if (g_cg->ind >= g_cg->end)
        vm_error("exceed function max size");

    if ((i >> 16))
        ott(i);
    else
        ot((uint16_t)i);
}

uint32_t thumb_gen::stuff_const(uint32_t op, uint32_t c)
{
    int try_neg = 0;
    uint32_t nc = 0, negop = 0;

    switch (op & 0x1f00000) {
    case 0x1000000:     // add
    case 0x1a00000:     // sub
        try_neg = 1;
        negop = op ^ 0xa00000;
        nc = -c;
        break;
    case 0x0400000:     // mov or eq
    case 0x0600000:     // mvn
        if (ANDEQ(op, 0xf0000)) { // mov
            try_neg = 1;
            negop = op ^ 0x200000;
            nc = ~c;
        }
        else if (c == ~0) { // orr
            return (op & 0xfff0ffff) | 0x1E00000;
        }
        break;
    case 0x800000:      // xor
        if (c == ~0)
            return (op & 0xe0100f00) | ((op >> 16) & 0xf) | 0x4f0000;
        break;
    case 0:             // and
        if (c == ~0)
            return (op & 0xe0100f00) | ((op >> 16) & 0xf) | 0x6f0000;
    case 0x0200000:     // bic
        try_neg = 1;
        negop = op ^ 0x200000;
        nc = ~c;
        break;
    }

    do {
        /* A6.3.2 */
        uint32_t m;
        int i, c1, c2, h;
        if (c < 256)
            return op | c;
        c1 = c & 0xff;
        if ((c2 = (c1 | (c1 << 16))) == c)
            return op | c | 0x0100;
        if ((c2 << 8) == c)
            return op | c | 0x0200;
        if ((c1 | c2) == c)
            return op | c | 0x0300;
        for (i = 8; i < 32; i ++) {
            m = 0xff << (32 - i);
            h = 1 << (39 - i);
            if ((c & m) == c && (c & h) == h)
                return op | imm_map(i, 4, 1, 26) | imm_map(i, 1, 3, 12) | imm_map(i, 0, 1, 7) | (((c) >> (32 - i)) & 0x7f);
        }
        op = negop;
        c = nc;
    } while (try_neg--);

    return 0;
}

/* A7.4.6 */
struct {
    int         op;
    int         cmode;
    uint64_t    mask;
    uint64_t    val;
    int         dt;
} simd_imm_tab[] = {
    { 0, 0b0000, 0,                    0x000000ff000000ff, 32 },
    { 0, 0b0010, 0,                    0x0000ff000000ff00, 32 },
    { 0, 0b0100, 0,                    0x00ff000000ff0000, 32 },
    { 0, 0b0110, 0,                    0xff000000ff000000, 32 },
    { 0, 0b1000, 0,                    0x00ff00ff00ff00ff, 16 },
    { 0, 0b1010, 0,                    0xff00ff00ff00ff00, 16 },

    { 0, 0b1100, 0x000000ff000000ff,   0x0000ff000000ff00, 32 },
    { 0, 0b1101, 0x0000ffff0000ffff,   0x00ff000000ff0000, 32 },

    { 0, 0b1110, 0,                    0xffffffffffffffff, 8 },
    { 0, 0b1111, 0,                    0xffffffffffffffff, 8 },

    { 1, 0b1110, 0,                    0xffffffffffffffff, 64 },
};

/* vector */
uint32_t stuff_constv(uint32_t op, uint64_t c)
{
    int bw = 64, i, j, a = 0, dt;
    uint64_t c0 = c, mask;

    for (i = 0; i < (count_of_array(simd_imm_tab) - 1); i++) {
        if (!ANDEQ(simd_imm_tab[i].mask, c)) continue;
        if ((~simd_imm_tab[i].val & c) != c) continue;
        dt = simd_imm_tab[i].dt;
        mask = (1ll << dt) - 1;

        for (bw = 64, c0 = c; bw; bw -= dt, c0 >>= dt) {
            if ((c0 & mask) != (c & mask)) break;
        }
        if (bw) continue;

        a = (c >> nlz((uint32_t)simd_imm_tab[i].val)) & 0xff;
        break;
    }

    if (i == (count_of_array(simd_imm_tab) - 1)) {
        for (i = 0; i < 8; i++) {
            j = (c >> (i * 8)) & 0xff;
            if (j && (j < 255)) return 0;
            a |= (j & 1) << i;
        }
        i = count_of_array(simd_imm_tab) - 1;
    }

    return op | imm_map(a, 7, 1, 28) | imm_map(a, 4, 3, 16) | (a & 0xf) | (simd_imm_tab[i].op << 5) | (simd_imm_tab[i].cmode << 8);
}

/* 这里的m是规定c的bitwidth ，假如为0，则默认的12，因为大部分的数都是12位填法 */
static uint32_t stuff_constw(uint32_t op, uint32_t c, int m)
{
    if (m == 0) m = 12;
    if (c >= (1 << m)) return 0;

    op |= imm_map(c, 11, 1, 26) | imm_map(c, 8, 3, 12) | imm_map(c, 0, 8, 0);
    if (m == 16) op |= imm_map(c, 12, 4, 16);
    return op;
}

void thumb_gen::stuff_const_harder(uint32_t op, uint32_t v)
{
    uint32_t x;
    x = stuff_const(op, v);
    if (x)
        o(x);
    else
    {
        /*
        当我们
        add r0, r1, c
        时，假如c过大，那么如下处理
        op: add r0, r1, c & 0xff
        o2: add r0, r0, c & 0xff00
        o2: add r0, r0, c & 0xff0000
        o2: add r0, r0, c & 0xff000000
        */
        uint32_t a[24], o2, no; 
        int i;
        a[0] = 0xff << 24;
        /* o2的源操作和目的操作数需要修正成同一个 */
        o2 = (op & 0xfff0ffff) | (op & 0x0f00);
        no = o2 ^ 0xa00000;
        for (i = 1; i < 24; i++)
            a[i] = a[i - 1] >> 1;

        o(stuff_const(op, v & 0xff));
        if (!(v & 0x8000) || !(v & 0x800000) || !(v & 0x80000000)) vm_error("stuff_const cant fit instruction");

        o(stuff_const(o2, v & 0xff00));
        if (!(v & 0x8000)) 
            o(stuff_const(no, v & 0x8000));
        o(stuff_const(o2, v & 0xff0000));
        if (!(v & 0x800000))
            o(stuff_const(no, v & 0x800000));
        o(stuff_const(o2, v & 0xff000000));
        if (!(v & 0x80000000))
            o(stuff_const(no, v & 0x80000000));
    }
}

int _push(int32_t reglist)
{
#define T1_REGLIST_MASK             0x40ff         
#define T2_REGLIST_MASK             0x5fff

    if (ANDNEQ(reglist, T1_REGLIST_MASK)) 
        o(0xb400 | (reglist & 0xff) | ((reglist & 0x4000) ? 0x100:0));
    else if (ANDNEQ(reglist, T2_REGLIST_MASK)) 
        o(0xe92d0000 | reglist);
    return 0;
}

/* A8.8.368 */
void _vpush(int s, uint32_t reglist)
{
    uint32_t n = ntz(reglist), bc = bitcount(reglist), b = reglist >> n, x = 0;

    if (((b + 1) & b))
        vm_error("reglist[%x] must continuous 1 bit", reglist);

    if (s) { // T2
        x = 0xed2d0a00 | imm_map(n, 4, 1, 22) | imm_map(n, 0, 4, 12) | bc;
    }
    else { // T1
        if ((bc & 1) || (n & 1))
            vm_error("reglist[%x] bitcount must event 1 bit", reglist);

        n /= 2;
        x = 0xed2d0b00 | imm_map(n, 4, 1, 22) | imm_map(n, 0, 4, 12) | bc;
    }
    o(x);
}

void _pop(int32_t reglist)
{
    /* A8.8.131 */
    if (ANDNEQ(reglist, 0x80ff)) {
        if (bitcount(reglist) < 1) UNPREDICITABLE();
        o(0xbc00 | (reglist & 0xff) | ((reglist & 0x8000) ? 0x100 : 0));
    }
    else if (ANDNEQ(reglist, 0xdfff)) {
        if (bitcount(reglist) < 2 || ANDEQ(reglist, 0xc000)) UNPREDICITABLE();
        o(0xe8bd0000 | (reglist & 0xdfff));
    }
}

void _xor_reg(int rd, int rn, int rm)
{
    if (rd == rn && rm <= 7 && rn <= 7)
		o(0x4040 | (rm << 3) | rd);	// Encoding t1
	else if (rd != 13 && rd != 15 && rn != 13 && rn != 15) 
        o(0xea800000 | (rn << 16) | (rd << 8) | rm);
	else
		vm_error ("_xor_reg invalid parameters");
}

/* A8.8.238 */
void _teq_reg(int rn, int rm, int shift)
{
    if (shift) UNPREDICITABLE();

    o(0xea900f00 | (rn << 16) | rm);
}

/* A8.8.123 */
void _or_reg(int rd, int rn, int rm, int setflags)
{
    if (setflags && (rd == rn) && rm < 8 && rn < 8)
        o(0x4300 | (rm << 3) | rd);
    else if (rd != 13 && rd != 15 && rn != 13 && rm != 13 && rm != 15) 
        o(0xea400000 | (rn << 16) | (rd << 8) | rm | (setflags << 20));
	else
		vm_error ("internal error: th_orr_reg invalid parameters\n");
}

enum SRType {
    SRType_LSL,
    SRType_LSR,
    SRType_ASR,
    SRType_ROR
};

void _adr(int rd, int imm)
{
    int x = abs(imm), add = imm >= 0;

    if (x >= 4096) return;

    if (align4(imm) && (imm < 1024) && (rd < 8))
        o(0xa000 | (rd << 8) || (imm >> 2));
    else if (!add)
        o(0xf2af0000 | (rd << 8) | imm_map(x, 11, 1, 26) | imm_map(x, 8, 3, 12) | imm_map(x, 0, 8, 0));
    else
        o(0xf20f0000 | (rd << 8) | imm_map(x, 11, 1, 26) | imm_map(x, 8, 3, 12) | imm_map(x, 0, 8, 0));
}

/* A8.8.6 */
void _add_reg(int rd, int rn, int rm, enum SRType sh_type, int shift)
{
    if (!shift && rd == rn) // t2
        o(0x4400 | (rm << 3) | (rd & 7) | (rd >> 3 << 7));
    else if (!shift && rn < 8 && rm < 8 && rd < 8) // t1
        o(0x1800 | (rm << 6) | (rn << 3) | rd);
    else
        o(0xeb000000 | (rn << 16) | (rd << 8) | rm | (sh_type << 4) | ((shift & 3) << 6) | (shift >> 2 << 12));
}

/* A8.8.4 */
void _add_imm(int rd, int rn, int imm)
{
    int a = abs(imm), add = imm >= 0;

    if (add && imm < 8 && rd < 8 && rn < 8)
        o(0x1c00 | (imm << 6) | (rn << 3) | rd); // t1
    else if (add && imm < 256 && rd < 8 && (rd == rn))
        o(0x3000 | (rd << 8) | imm); // t2
    else if (add && imm < 4096)
        o(0xf2000000 | (rn << 16) | (rd << 8) | imm_map(imm, 11, 1, 26) | imm_map(imm, 8, 3, 12) | imm_map(imm, 0, 8, 0));
    else {
        uint32_t x = thumb_gen::stuff_const(0, imm);
        if (x) {
            o(0xf1000000 | x | (rn << 16) | (rd << 8));
        }
    }
}

int thumb_gen::_add_sp_imm(int rd, int rn, uint32_t imm)
{
    /* A8.8.9 */
    if (rn == SP) {
        if (align4(imm) && (imm < 1024) && (rd < 8))
            o(0xa800 | (rd << 8) | (imm >> 2));
        else if ((rd == SP) && align4(imm) && (imm < 512))
            o(0xb000 | (imm >> 2));
        else if (imm < 4096)
            o(stuff_constw(0xf20d0000 | (rd << 8), imm, 12));
        else
            stuff_const_harder(0xf10d0000, imm);
    }

    return 0;
}

/* A8.8.19 */
void _bfc(int rd, int lsb, int width)
{
    int msb = lsb + width - 1;

    o(0xf36f0000 | (rd << 8) | imm_map(lsb, 3, 2, 12) | imm_map(lsb, 0, 2, 6) | msb);
}

/* A8.8.221 */
void thumb_gen::_sub_imm(int rd, int rn, uint32_t imm)
{
    if ((imm < 8) && (rd < 8) && (rn < 8)) // t1
        o(0x1e00 | (imm << 6) | (rn << 3) | rd);
    else if (imm < 256 && (rd == rn) && (rn < 8)) // t2
        o(0x3800 | imm | (rn << 8));
    else if (imm <= 0x0fff && rd != 13 && rd != 15) { // t4
        o(stuff_constw(0xf2a00000 | (rn << 16) | (rd << 8), imm, 0));
    }
    else
        stuff_const_harder(0xf1a00000 | (rn << 16) | (rd << 8), imm);
}

void thumb_gen::_sub_sp_imm(int imm)
{
    if (!(imm & 3)) /* ALIGN 4 */
        o(0xb080 | (imm >> 2));
    else if (imm < 4096) //subw, T3
        o(stuff_constw(0xf25b0000, imm, 12));
    else // T2
        o(stuff_const(0xf1ad0000 | (SP << 8), imm));
}

void _sub_reg(int rd, int rn, int rm, SRType shtype, int sh)
{
    if (!sh && rd < 8 && rn < 8 && rm < 8)
        o(0x1a00 | (rm << 6) | (rn << 3) | rd);
    else
        o(0xeba00000 | (rn << 16) | (rd << 8) | rm | (shtype << 4) | ((sh & 3) << 6) | (sh >> 2 << 12));
}

void _cmp_reg(int rn, int rm)
{
    if (rn < 8 && rm < 8)
        o(0x4280 | (rm << 3) | rn);
    else
        o(0x4500 | (rm << 3) | (rn & 7) | ((rn >> 3) << 7));
}


void thumb_gen::_cmp_imm(int rn, uint32_t imm)
{
    uint32_t x, rm;

    if ((imm < 256) && (rn < 8))
        o(0x2800 | (rn << 8) | imm);
    else if ((x = stuff_const(0, imm)))
        o(0xf1b00f00 | x | (rn << 16));
    else {
        rm = regalloc(curp);
        _mov_imm(rm, imm);
        _cmp_reg(rn, rm);
    }
}

/* A8.8.72 */
void _ldrd_imm(int rt, int rt2, int rn, int imm)
{
    int add = imm > 0;
    imm = abs(imm);

    if (rn == rt) UNPREDICITABLE();
    if (!align4(imm)) UNPREDICITABLE();

    o(0xe8550000 | (imm ? 0x01000000:0) | (rn << 16) | (rt << 12) | (rt2 << 8) | (imm >> 2) | (add << 23));
    /* wback ? */
}

void _ldr_lit(int rt, int imm)
{
}

/* A8.8.62 */
void _ldr_imm(int rt, int rn, int imm, int wback)
{
    int x = abs(imm), add = imm >= 0, p = imm != 0;

    if (!wback && add && align4(imm) && rt < 8) {
        if (rn == SP && imm < 1024) {
            o(0x9800 | (rt << 8) | (imm >> 2));     // T1
            return;
        }
        else if (rn < 8 && imm < 128) {
            o(0x6800 | ((imm >> 2) << 6) | (rn << 3) | rt);     // T2
            return;
        }
    }

    if (!wback && add && imm < 4096)
        o(0xf8d00000 | (rn << 16) | (rt << 12) | imm);  // T3
    else if (x < 256)
        o(0xf8500800 | (rn << 16) | (rt << 12) | (add << 9) | x | (wback << 8) | (p << 10)); // T4
}

void _ldr_reg(int rt, int rn, int rm, int lsl)
{
    if (!lsl && rt < 8 && rn < 8 && rm < 8)
        o(0x5800 | (rm << 6) | (rn << 3) | rt);
    else
        o(0xf8500000 | (rn << 16) | (rt << 12) | (lsl << 4) | rm);
}

/* A8.8.67 */
void _ldrb_imm(int rt, int rn, int imm, int wback)
{
    int x = abs(imm), add = imm >= 0, p = 1;

    if (add && imm < 32 && rn < 8 && rt < 8)
        o(0x7800 | (imm << 6) | (rn << 3) | rt);
    else if (add && (imm < 4096))
        o(0xf8900000 | (rn << 16) | (rt << 12) | imm);
    else if (x < 256)
        o(0xf8100800 | (rn << 16) | (rt << 12) | x | (p << 10) | (add << 9) | (wback << 8));
}

void _ldrb_reg(int rt, int rn, int rm, int lsl)
{
    if (rt < 8 && rn < 8 && rm < 8 && !lsl)
        o(0x5c00 | (rm << 6) | (rn << 3) | rt);
    else
        o(0xf811 | (rt << 12) | (rn << 16) | rm | (lsl << 4));
}

/* A8.8.94 */
void _lsl_imm(int rd, int rm, int imm)
{
    if (imm >= 32) return;

    if (rm < 8 && rd < 8)
        o((imm << 6) | (rm << 3) | (rd << 3));
    else
        o(0xea4f0000 | (rd << 8) | rm | imm_map(imm, 2, 3, 12) | imm_map(imm, 0, 2, 6));
}

/* A8.8.96 */
void _lsr_imm(int rd, int rm, int imm)
{
    if (imm >= 32) return;

    if (rm < 8 && rd < 8)
        o(0x0800 | (imm << 6) | (rm << 3) | rd);
    else
        o(0xea4f0010 | (rd << 8) | rm | imm_map(imm, 2, 3, 12) | imm_map(imm, 0, 2, 6));
}

/* A8.8.100 */
void _mla(int rd, int rn, int rm, int ra)
{
    o(0xfb000000 | (rn << 16) | (ra << 12) | (rd << 8) | rm);
}

/* A8.8.114 */
void _mul(int rd, int rn, int rm)
{
    if (rd < 8 && rn < 8 && (rd == rm))
        o(0x4340 | (rn << 3) | rd);
    else
        o(0xfb00f000 | (rn << 16) | (rd << 8) | rm);
}

void _rsb_imm(int rd, int imm, int rn)
{
    uint32_t x;

    if (rd < 8 && rn < 8 && !imm)
        o(0x4240 | (rn << 3) | rd);
    else if (x = thumb_gen::stuff_const(0, imm))
        o(0xf1c00000 | x | (rn << 16) | (rd << 8));
}

void _rsb_reg(int rd, int rn, int rm, SRType shtype, int shift)
{
    o(0xebc00000 | (rn << 16) | (rd << 8) | rm | (shtype << 4) | imm_map(shift, 2, 3, 12) | imm_map(shift, 0, 2, 6));
}

void _str(int rt, int rn, int rm, int imm)
{
    if (rm != -1) {
    }
    else if (rt < 8 && rn < 8) {
        if (align4(imm) && imm < 128)
            o(0x6000 | ((imm >> 2) << 6) | (rn << 3) | rt);
    }
    else if ((rn == SP) && rt < 8 && align4(imm) && imm < 1024)
        o(0x9000 | (rt << 8) | (imm >> 2));
    else if (imm < 4096)
        o(0xf8600000 | (rn << 16) | (rt << 12) | imm);
}

void _str_reg(int rt, int rn, int rm, int lsl)
{
    if (!lsl && rt < 8 && rn < 8 && rm < 8)
        o(0x5000 | (rm << 6) | (rn << 3) | rt);
    else 
        o(0xf8400000 | (rn << 16) | (rt << 12) | (lsl << 4) | rm);
}

/* A8.8.203 */
void _str_imm(int rt, int rn, int imm, int wback)
{
    int add = imm >= 0, pos = abs(imm), p = pos >= 0;

    if (add && align4(imm) && imm < 128 && rt < 8 && rn < 8) // t1
        o(0x6000 | (imm >> 2 << 6) | rt | (rn << 3));
    else if (add && (rn == SP) && rt < 8 && align4(imm) && (imm < 1024)) // t2
        o(0x9000 | (rt << 8) | (imm >> 2));
    else if (add && imm < 4096) // t3
        o(0xf8c00000 | (rn << 16) | (rt << 12) | imm);
    else if (pos < 128)
        o(0xf8400800 | imm | (rt << 12) | (rn << 16) | (p << 10) | (add << 9) | (wback << 8));
}

void _strd(int rt, int rt2, int rn, int imm, int w)
{
    int add = imm >= 0, pos = abs(imm), p = imm != 0;

    if (align4(pos) && (pos < 1024))
        o(0xe8400000 | (pos >> 2) | (add << 23) | (rn << 16) | (rt << 12) | (rt2 << 8) | (1 << 24) | (w << 21));
}

/* A8.8.206 

@w  writeback
*/
void _strb_imm(int rt, int rn, int imm, int w)
{
    int x, u;

    if (imm >= 0 && imm < 32 && rt < 8 && rn < 8) // t1
        o(0x7000 | (rn << 3) | rt | (imm << 6));
    else if (imm >= 0 && imm < 4096) // t2
        o(0xf8800000 | (rn << 16) | (rt << 12) | imm);
    else if (imm > -255 && imm < 256) { // T3
        x = abs(imm);
        u = imm >= 0;
        /* op | P | */
        o(0xf8000800 | (1 << 10) | (rn << 16) | (rt << 12) | x | (w << 8) | (u << 9));
    }

}

void thumb_gen::_mov_imm(int rd, uint32_t v)
{
    /* A8.8.102 */
    if (in_imm8(v) && in_imm3(rd)) 
        o(0x2000 | (rd << 8) | v); // T1
    else {
        uint32_t x = stuff_const(0xf04f0000 | (rd << 8), v); // T2
        if (x) {
            o(x);
            return;
        }

        /* 
        碰见超大数
        mov rd, v.lo
        movt rd, v.hi
        */
        o(stuff_constw(0xf2400000 | (rd << 8), v & 0xffff, 16));
        v >>= 16;
        o(stuff_constw(0xf2c00000 | (rd << 8), v, 16));
    }
}

/* A8.8.101 */
void _mls(int rd, int rn, int rm, int ra)
{
    o(0xfb000010 | (rn << 16) | (ra << 12) | (rd << 8) | rm);
}

/* A8.8.339 */
void vmov_imm(int siz, int vd, uint64_t imm)
{
    /* T1 , vd是以4字节为单位的，当传入后，需要转换到 Dd, Qd为单位 */
    uint32_t x = 0xef800010, d = vd / 2;
    if (siz == 16) x |= (1 << 6);
    o(stuff_constv(x, imm) | imm_map(d, 4, 1, 22) | imm_map(d, 0, 4, 12));
}

uint32_t encbranch(int pos, int addr, int fail)
{
    /* A8.8.18 */
    addr -= pos + 4;
    addr /= 2;
    if ((addr >= 0xEFFF) || addr < -0xEFFF) {
        vm_error("FIXME: function bigger than 1MB");
        return 0;
    }
    return 0xf0008000 | imm_map(addr, 31, 1, 26) | imm_map(addr, 18, 1, 11) | imm_map(addr, 17, 1, 13) | imm_map(addr, 11, 6, 16) | imm_map(addr, 0, 11, 0);
}

uint32_t encbranch2(int pos, int addr, int arm)
{
    addr -= pos + 4;
    addr /= 2;
    if ((addr >= 0xffffff) && (addr < -0xffffff))
        vm_error("FIXME: jmp bigger than 16MB");

    int s = bit_get(addr, 31, 1), j1, j2, i1 = bit_get(addr, 22, 1), i2 = bit_get(addr, 21, 1);

    j1 = (!i1) ^ s;
    j2 = (!i2) ^ s;

    return  (s << 26) | (j1 << 13) | (j2 << 11) | imm_map(addr, 11, 10, 16) | imm_map(addr, 0, 11, 0);
}

char *vstl_slgh[] = {
    "ma:4 = copy rn:4" ,
    ""
};

pit thumb_gen::g_vstl(flowblock *b, pit pit)
{
    return retrieve_orig_inst(b, pit, 1);
}

pit thumb_gen::g_vpop(flowblock *b, pit pit)
{
    return retrieve_orig_inst(b, pit, 1);
}

void thumb_gen::topologsort()
{
}

int thumb_gen::run()
{
    int i;
    uint32_t x;

    preprocess();
    //fd->bblocks.dump_live_set(fd->find_op(SeqNum(Address(), 2170)));
    fd->dump_cfg(fd->name, "exe_pre", 1);

    /* 对block进行排序 

    */
    sort_blocks(blist);

    /* 针对每一个Block生成代码，但是不处理末尾的jmp */
    for (i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];
        run_block(b, i);

        if (b->is_rel_branch()) {
            /* FIXME:bblock的start_addr起始和first op的addr可能不是一回事，一般出现在复制的情况下 */
            while (blist[i + 1]->first_op()->get_addr() == b->first_op()->get_addr()) i++;
        }
    }

    /* 修复末尾的跳转 */
    printf("fix jmp list\n");
    for (i = 0; i < flist.size(); i++) {
        fix_item *item = flist[i];

        /* 读取出原先的inst，然后取出其中的cond，生成新的inst，然后写入buf */
        if (item->cond == COND_AL) {
            x = 0xf0009000;
            x |= encbranch2(item->from, item->to_blk->cg.data - data, 0);
        }
        else {
            x = read_thumb2(data + item->from);
            x =  (x & 0x03c00000)| encbranch(item->from, item->to_blk->cg.data - data, 0);
        }
        write_thumb2(x, data + item->from);
        //dump_one_inst(item->from, NULL);
    }
    printf("fix jmp list success\n");

    return 0;
}

void thumb_gen::save(void)
{
}

int thumb_gen::run_block(flowblock *b, int b_ind)
{
    vector<fix_vldr_item> fix_vlist;
    list<pcodeop *>::iterator it, it1;
    pcodeop *p, *p1, *p2, *p3, *p4, *tp;
    uint32_t x, rt, rd, rn, rm, setflags;
    pc_rel_table pc_rel_tab;
    int oind, imm, target_thumb, i;

    b->cg.data = data + ind;

    for (it = b->ops.begin(); it != b->ops.end(); ++it) {
        curp = p = *it;

        /* branch 指令不处理，由外层循环进行填充 */
        if (p->opcode == CPUI_BRANCH) continue;
        /* phi节点不处理 */
        if (p->opcode == CPUI_MULTIEQUAL) continue;

        /* 这一坨代码都是相当于词法分析的token的预取 */
        it1 = it;
        p1 = p2 = p3 = NULL;
        ++it1;
        if (it1 != b->ops.end())
            p1 = *it1++;
        if (it1 != b->ops.end())
            p2 = *it1++;
        if (it1 != b->ops.end())
            p3 = *it1++;

        oind = ind;
        setflags = 0;

        if (dobc::singleton()->is_simd(p->get_addr())) {
            if (p->output && p->output->is_pc_constant())
                fix_vlist.push_back(fix_vldr_item(p, p3, ind));
            it = retrieve_orig_inst(b, it, 1);
            goto inst_label;
        }

        switch (p->opcode) {
        case CPUI_COPY:
            /* push */
            if (poa(p) == ama) {
                if ((pi0a(p) == asp) && p1) {
                    if (p1->opcode == CPUI_LOAD)
                        it = g_pop(b, it);
                    else
                        it = g_push(b, it);
                }
                else if (d->is_greg(pi0a(p)))
                    it = g_vstl(b, it);
            }
            else if (poa(p) == alr) {
                if ((p1->opcode == CPUI_CALL) && (pi0(p1)->get_addr().getSpace() == d->ram_spc)) {
                    imm = pi0(p1)->get_addr().getOffset();
                    target_thumb = d->func_is_thumb(pi0(p1)->get_val());
                    /* A8.8.25 */
                    if (fd->flags.thumb ^ target_thumb)
                        o(0xf000c000 | encbranch2(ind, imm, !target_thumb));
                    else 
                        o(0xf000d000 | encbranch2(ind, imm, !target_thumb));
                    advance(it, 1);
                }
                else if (pi0(p)->is_constant()) {
                    _mov_imm(LR, pi0(p)->get_val());
                }
            }
            else if (pi0(p)->is_hard_constant()) {
                if (isreg(p->output))
                    _mov_imm(reg2i(poa(p)), pi0(p)->get_val());
                else if (istemp(p->output)){
                    if (isreg(p1->output)) {
                        switch (p1->opcode) {
                        case CPUI_COPY:
                            _mov_imm(reg2i(poa(p1)), pi0(p)->get_val());
                            break;

                        case CPUI_LOAD:
                            /* FIXME:假如load指令调用了一个常数，那么它一定是相对pc寄存器的？ */
                            rn = reg2i(poa(p1));
                            imm = pi0(p)->get_val();
                            /*
                            pc - (ind) - 4
                            */
                            _mov_imm(rn, imm - (ind + 4 * (stuff_const(0, imm) ? 1:2)) - 4);

                            _add_reg(rn, rn, PC, SRType_LSL, 0);
                            _ldr_imm(rn, rn, 0, 0);
                            break;

                        case CPUI_INT_ADD:
                            rn = reg2i(pi0a(p1));
                            if (rn == SP)
                                _add_sp_imm(reg2i(poa(p1)), rn, pi0(p)->get_val());
                            else
                                _add_imm(reg2i(poa(p1)), rn, pi0(p)->get_val());
                            break;

                        case CPUI_INT_SUB:
                            if (p->output == pi0(p1))
                                _rsb_imm(reg2i(poa(p1)), pi0(p)->get_val(), reg2i(pi1a(p1)));
                            else
                                _sub_imm(reg2i(poa(p1)), reg2i(pi0a(p1)), pi0(p)->get_val());
                            break;

                        case CPUI_INT_AND:
                            int lsb, width;
                            if (continuous_zero_bits(pi0(p)->get_val(), &lsb, &width)) {
                            }
                            else {
                                _bfc(reg2i(poa(p1)), lsb, width);
                            }
                            break;
                        }
                        advance(it, 1);
                    }
                    else if (istemp(p1->output)) {
                        if (p2->opcode == CPUI_INT_2COMP) {
                            it = retrieve_orig_inst(b, it, 1);
                        }
                    }
                    else if (d->is_tsreg(poa(p1))) {
                        if (p1->opcode == CPUI_INT_SBORROW) {
                            it = retrieve_orig_inst(b, it, 1);
                        }
                    }
                    else if (d->is_vreg(poa(p1))) {
                        vmov_imm(p1->output->get_size(), d->vreg2i(poa(p1)), pi0(p)->get_val());
                        advance(it, 1);
                    }
                }
                else if (d->is_vreg(poa(p))) {
                    int size = p->output->get_size();
                    if (p1 && d->is_vreg(poa(p1)) && pi0(p1)->is_constant() && pi0(p)->get_val() == pi0(p1)->get_val()) {
                        size += p1->output->get_size();
                        advance(it, 1);
                    }
                    vmov_imm(size, d->vreg2i(poa(p)), pi0(p)->get_val());
                }
            }
            else if (pi0(p)->is_hard_pc_constant()) {
                if (isreg(p->output)) {
                    _adr(reg2i(poa(p)), 0);
                    pc_rel_tab[pi0(p)->get_val()] = p;
                }
            }
            else if (istemp(p->output)) {
                if (p1->opcode == CPUI_INT_ADD)
                    _add_reg(reg2i(poa(p1)), reg2i(pi0a(p1)), reg2i(pi0a(p)), SRType_LSL, 0);
                else if (p1->opcode == CPUI_INT_SUB)
                    _sub_reg(reg2i(poa(p1)), reg2i(pi0a(p1)), reg2i(pi0a(p)), SRType_LSL, 0);
                it = advance_to_inst_end(it);
            }
            else if (isreg(pi0(p))) {
                if (isreg(p->output)) {
                    rd = reg2i(poa(p));
                    /* A8.8.103*/
                    o(0x4600 | (reg2i(pi0a(p)) << 3) | (rd & 7) | (rd >> 3 << 7));
                }
                else if (istemp(p->output)) {
                    switch (p1->opcode) {
                    case CPUI_INT_SUB:
                        _sub_reg(reg2i(poa(p1)), reg2i(pi0a(p1)), reg2i(pi0a(p)), SRType_LSL, 0);
                        break;
                    }

                    it = advance_to_inst_end(it);
                }
            }
            break;

        case CPUI_LOAD:
            if (isreg(p->output) && pi1(p)->is_constant()) {
                rd = reg2i(poa(p));
                _mov_imm(rd, pi1(p)->get_val());
                o(0xf8d00000 | (rd << 12) | (rd << 16) | 0);
            }
            break;

        case CPUI_INT_NOTEQUAL:
        case CPUI_INT_EQUAL:
            if (istemp(p->output) ) {
                /* A8.3 

                FIXME:这一整块都需要重写，需要依赖根据 A8.3 的规则，自动计算，而不能简单的Pattern进行匹配
                */
                if ((pi0a(p) == azr) && pi1(p)->is_constant()) {
                    imm = pi1(p)->get_val();
                    /*

                mov.eq r1, xxxx 转换以后

                    t0 = INT_NOTEQUAL zr, 0
                    t1 = BOOL_NEGATE t0
                    cbranch true_label t1
         false_label:


          true_label: 

                    这个是ne


                    warn: sleigh在转it指令时，mov.后面的条件，走的是false的分支，为真的条件实际上是相反的。
                    */
                    if (!imm && (p1->opcode == CPUI_BOOL_NEGATE) && (p2->opcode == CPUI_CBRANCH)) {
                        write_cbranch(b, ((p->opcode == CPUI_INT_NOTEQUAL)?COND_NE:COND_EQ));
                    }
                    else if (imm && p1->opcode == CPUI_CBRANCH) {
                        write_cbranch(b, ((p->opcode == CPUI_INT_NOTEQUAL)?COND_NE:COND_EQ));
                    }
                    else if (!imm && p1->opcode == CPUI_CBRANCH) {
                        write_cbranch(b, ((p->opcode == CPUI_INT_NOTEQUAL)?COND_EQ:COND_NE));
                    }
                }
                else if ((pi0a(p) == acy) && pi1(p)->is_constant()) {
                    imm = pi1(p)->get_val();
                    if (p->opcode == CPUI_INT_NOTEQUAL) imm = !imm;
                    if (p1->opcode == CPUI_BOOL_NEGATE) imm = !imm;

                    write_cbranch(b, imm?COND_CS:COND_CC);

                }
                else if ((pi0a(p) == ang) && pi1(p)->is_hard_constant()) {
                    imm = pi1(p)->get_val();
                    if (p->opcode == CPUI_INT_NOTEQUAL) imm = !imm;
                    if (p1->opcode == CPUI_BOOL_NEGATE) imm = !imm;
                    write_cbranch(b, imm ? COND_MI:COND_PL);
                }
                else if (pi0a(p) == ang && pi1a(p) == aov && p1->opcode == CPUI_BOOL_NEGATE && p2->opcode == CPUI_CBRANCH) {
                    write_cbranch(b, (p->opcode == CPUI_INT_EQUAL) ? COND_LT:COND_GE);
                }
                else if ((p1->opcode == CPUI_BOOL_OR) && (p2->opcode == CPUI_BOOL_NEGATE) && (p3->opcode == CPUI_CBRANCH)) {
                    if ((pi0a(p) == ang) && (pi1a(p) == aov) && (pi0a(p1) == azr)) {
                        write_cbranch(b, COND_GT);
                    }
                }

                it = advance_to_inst_end(it);
            }
            break;

        case CPUI_INT_SEXT:
            it = retrieve_orig_inst(b, it, 1);
            break;

        case CPUI_INT_SUB:
            if (poa(p) == ama) it = g_push(b, it);
            else if (istemp(p->output)) {
                rn = d->reg2i(pi0a(p));

                switch (p1->opcode) {
                case CPUI_LOAD:
                    if ((p1->output->size == 1) && istemp(p1->output)) {
                        _ldrb_imm(reg2i(poa(p2)), reg2i(pi0a(p)), - pi1(p)->get_val(), 0);
                        advance(it, 2);
                    }
                    break;

                case CPUI_INT_EQUAL:
                    /* A8.8.37 */
                    if (pi1(p)->is_constant()) 
                        _cmp_imm(rn, pi1(p)->get_val());
                    /* A8.8.38 */
                    else if (isreg(pi0(p)) && isreg(pi1(p))) {
                        rm = d->reg2i(pi1a(p));
                        _cmp_reg(rn, rm);
                    }

                    it = advance_to_inst_end(it);
                    break;

                case CPUI_INT_SLESS:
                    it = retrieve_orig_inst(b, it, 1);
                    break;

                case CPUI_SUBPIECE:
                    if (istemp(p1->output) && p2->opcode == CPUI_STORE) {
                        _strb_imm(reg2i(pi0a(p1)), rn, -pi1(p)->get_val(), 0);
                        advance(it, 2);
                    }
                    break;
                }
            }
            else if (isreg(p->output)) {
                if (pi0(p)->is_hard_constant()) {
                    _rsb_imm(reg2i(poa(p)), pi0(p)->get_val(), reg2i(pi1a(p)));
                }
                else if (isreg(pi0(p)) || (pi0a(p) == ama)) {
                    rd = reg2i(poa(p));
                    rn = reg2i(pi0a(p));
                    if (isreg(pi1(p))) {
                        /* A8.8.223 */
                        rm = reg2i(pi1a(p));

                        _sub_reg(rd, rn, rm, SRType_LSL, 0);
                        it = advance_to_inst_end(it);
                    }
                    /* sub sp, sp, c 
                    A8.8.225.T1
                    */
                    else if (pi1(p)->is_constant()) {
                        if (((pi0a(p) == asp) || (pi0a(p) == ama)) && (poa(p) == asp)) {
                            if (p1 && p2 && (p1->opcode == CPUI_COPY) && (p2->opcode == CPUI_STORE) && d->is_vreg(pi2a(p2))) {
                                it = g_vpush(b, it);
                                goto inst_label;
                            }
                            _sub_sp_imm(pi1(p)->get_val());
                        }
                        else
                            _sub_imm(rd, rn, pi1(p)->get_val());
                    }
                }
            }
            break;

        case CPUI_INT_LESSEQUAL:
        case CPUI_INT_ZEXT:
            it = retrieve_orig_inst(b, it, 1);
            break;


        case CPUI_INT_ADD:
            if (istemp(p->output)) {
                switch (p1->opcode) {
                case CPUI_COPY:
                    if ((pi0a(p) == asp) && pi1(p)->is_constant() && a(pi0(p1)) == poa(p) && isreg(p1->output))
                        _add_sp_imm(reg2i(poa(p1)), SP, pi1(p)->get_val());
                    else if (istemp(p1->output)) {
                        if (p2 && (p2->opcode == CPUI_STORE)) {
                            if (pi2(p2)->size == 1) {
                                if (!d->is_greg(pi0a(p1))) {
                                    rt = regalloc(p1);
                                    _mov_imm(rt, pi0(p1)->get_val());
                                }
                                else
                                    rt = d->reg2i(pi0a(p1));
                                rn = d->reg2i(pi0a(p));
                                _strb_imm(rt, rn, pi1(p)->get_val(), 0);
                                advance(it, 1);
                            }
                            else if (p3 && (p3->opcode == CPUI_INT_ADD) && pi0(p3) == p1->output && (p4 = *it1)->opcode == CPUI_STORE) {
                                _strd(reg2i(pi2a(p2)), reg2i(pi2a(p4)), reg2i(pi0a(p)), pi1(p)->get_val(), 0);
                                advance(it, 3);
                            }
                        }
                    }
                    advance(it, 1);
                    break;

                case CPUI_STORE:
                    if (a(pi1(p1)) == poa(p)) {
                        if (p2 && p3 && (p2->opcode == CPUI_INT_ADD) 
                            && (p3->opcode == CPUI_STORE) && (pi0a(p2) == poa(p))) {
                            _strd(reg2i(pi2a(p1)), reg2i(pi2a(p3)), reg2i(pi0a(p)), pi1(p)->get_val(), 0);
                            advance(it, 2);
                        }
                        else
                            _str_imm(reg2i(pi2a(p1)), reg2i(pi0a(p)), pi1(p)->get_val(), 0);

                        advance(it, 1);
                    }
                    break;

                case CPUI_LOAD:
                    if (a(pi1(p1)) == poa(p)) {
                        if (p2 && p3 && (pi0a(p2) == poa(p)) && (p3->opcode == CPUI_LOAD)) {
                            _ldrd_imm(reg2i(poa(p1)), reg2i(poa(p3)), reg2i(pi0a(p)), p->get_in(1)->get_val());
                            advance(it, 3);
                        }
                        else if (istemp(p1->output) && isreg(p2->output) && p2->opcode == CPUI_INT_ZEXT) {
                            if (pi1(p)->is_hard_constant()) {
                                _ldrb_imm(reg2i(poa(p2)), reg2i(pi0a(p)), pi1(p)->get_val(), 0);
                            }
                            else
                                _ldrb_reg(reg2i(poa(p2)), reg2i(pi0a(p)), reg2i(pi1a(p)), 0);
                            advance(it, 2);
                        }
                        else {
                            rt = reg2i(poa(p1));
                            imm = pi1(p)->get_val();
                            /* A8.8.62 */
                            if ((pi0a(p) == asp) && align4(imm) && (imm < 1024) && (rt < 8)) // T2
                                o(0x9800 | (rt << 8) | (imm >> 2));
                            else if (imm < 4096) // T3
                                o(0xf8d00000 | (rt << 12) | (reg2i(pi0a(p)) << 16) | imm);

                            advance(it, 1);
                        }
                    }
                    break;

                case CPUI_SUBPIECE:
                    if (istemp(p1->output) && (p2->opcode == CPUI_STORE)) {
                        rt = reg2i(pi0a(p1));
                        rn = reg2i(pi0a(p));
                        _strb_imm(rt, rn, pi1(p)->get_val(), 0);
                        advance(it, 2);
                    }
                    break;

                case CPUI_INT_ADD:
                    if (pi0(p)->flags.from_pc && isreg(p1->output)) {
                        rd = reg2i(poa(p1));
                        imm = p1->output->get_val();
                        _mov_imm(rd, imm - (ind + 4 * (stuff_const(0, imm) ? 1:2)) - 4);
                        _add_reg(rd, rd, PC, SRType_LSL, 0);
                        advance(it, 1);
                    }
                    break;
                }
            }
            else if ((pi0a(p) == asp) && pi1(p)->is_constant()) {
                if (p1 && p1->opcode == CPUI_LOAD)
                    it = g_pop(b, it);
                else if (isreg(p->output))
                    _add_sp_imm(reg2i(poa(p)), SP, pi1(p)->get_val());
            }
            else if (isreg(p->output)) {
                rd = reg2i(poa(p));
                rn = reg2i(pi0a(p));
                /* A8.8.4 */
                if (pi1(p)->is_hard_constant()) {
                    imm = (int)pi1(p)->get_val();
                    _add_imm(rd, rn, imm);
                    it = advance_to_inst_end(it);
                }
                else if (isreg(pi1(p)))
                    _add_reg(reg2i(poa(p)), reg2i(pi0a(p)), reg2i(pi1a(p)), SRType_LSL, 0);
            }
            break;

        case CPUI_INT_SBORROW:
            if (d->is_tsreg(poa(p))) {
                if (p1->opcode == CPUI_INT_SUB && istemp(p1->output)) {
                    if (pi1(p)->is_hard_constant())
                        _cmp_imm(reg2i(pi0a(p)), pi1(p)->get_val());
                    else if (isreg(pi1(p)))
                        _cmp_reg(reg2i(pi0a(p)), reg2i(pi1a(p)));

                    advance(it, 2);
                    while ((tp = *it)->output && (d->is_sreg(poa(tp)) || d->is_tsreg(poa(tp)))) it++;
                    it--;
                }
            }
            break;


        case CPUI_INT_XOR:
            if (istemp(p->output)) {
                if (p1) {
                    if (d->is_sreg(poa(p1))) {
                        _teq_reg(reg2i(pi0a(p)), reg2i(pi1a(p)), 0);
                        advance(it, 1);
                    }
                    else if (p2 && d->is_tsreg(poa(p1)) && d->is_sreg(poa(p2))) {
                        _teq_reg(reg2i(pi0a(p)), reg2i(pi1a(p)), 0);
                        advance(it, 2);
                    }
                }
            }
            else 
                _xor_reg(reg2i(poa(p)), reg2i(pi0a(p)), reg2i(pi1a(p)));
            break;

        case CPUI_INT_AND:
            if (poa(p) == apc) {
                p1 = *++it;
                if (poa(p1) == alr) {
                    p2 = *++it;
                    if (p2->opcode == CPUI_CALLIND) 
                        o(0x4780 | (reg2i(pi0a(p)) << 3));
                }
            }
            else if (istemp(p->output)) {
                p1 = *++it;
                if (p1->opcode == CPUI_STORE) {
                    if (isreg(pi0(p)) && pi1(p)->is_constant()) {
                        imm = pi1(p)->get_val();
                        rt = reg2i(pi2a(p1));
                        rn = reg2i(pi0a(p));
                        _str(rt, rn, -1, imm);
                    }
                }
            }
            break;


        case CPUI_INT_OR:
            if (isreg(p->output)) {
                if (p1 && p2 && p1->opcode == CPUI_INT_EQUAL && p2->opcode == CPUI_COPY && d->is_tsreg(poa(p1)) && d->is_sreg(poa(p2))) {
                    advance(it, 2);
                    setflags = 1;
                }

                _or_reg(reg2i(poa(p)), reg2i(pi0a(p)), reg2i(pi1a(p)), setflags);
            }
            break;

        case CPUI_INT_LEFT:
            if (istemp(p->output)) {
                if (istemp(p1->output)) {
                    if (p1->opcode == CPUI_INT_ADD) {
                        if (p2->opcode == CPUI_LOAD) {
                            if (isreg(p2->output))
                                _ldr_reg(reg2i(poa(p2)), reg2i(pi0a(p1)), reg2i(pi0a(p)), pi1(p)->get_val());
                            else  if (isreg(p3->output))
                                it = retrieve_orig_inst(b, it, 1);
                        }
                        else if (p2->opcode == CPUI_STORE) {
                            _str_reg(reg2i(pi2a(p2)), reg2i(pi0a(p1)), reg2i(pi0a(p)), pi1(p)->get_val());
                        }
                    }
                    else if (p1->opcode == CPUI_COPY) {
                        if (p2->opcode == CPUI_INT_ADD) {
                            _add_reg(reg2i(poa(p2)), reg2i(pi0a(p2)), reg2i(pi0a(p)), SRType_LSL, pi1(p)->get_val());
                        }
                        else if (p2->opcode == CPUI_INT_SUB) {
                            _sub_reg(reg2i(poa(p2)), reg2i(pi0a(p2)), reg2i(pi0a(p)), SRType_LSL, pi1(p)->get_val());
                        }
                    }
                    it = advance_to_inst_end(it);
                }
            }
            else if (isreg(p->output)) {
                if (pi1(p)->is_hard_constant())
                    _lsl_imm(reg2i(poa(p)), reg2i(pi0a(p)), pi1(p)->get_val());
            }
            break;

        case CPUI_INT_RIGHT:
            if (isreg(p->output)) {
                _lsr_imm(reg2i(poa(p)), reg2i(pi0a(p)), pi1(p)->get_val());
            }
            else if (istemp(p->output)) {
                if (istemp(p1->output)) {
                    if (p1->opcode == CPUI_COPY) {
                        if (p2->opcode == CPUI_INT_ADD) {
                            _add_reg(reg2i(poa(p2)), reg2i(pi0a(p2)), reg2i(pi0a(p)), SRType_LSR, pi1(p)->get_val());
                        }
                    }
                }

                it = advance_to_inst_end(it);
            }
            break;

        case CPUI_INT_SRIGHT:
            /* NOTE：
            2021年5月15日，我从这个点开始，尝试把部分指令完全不处理，直接从原文中拷贝，但是因为我还没有考虑具体
            哪些指令可以拷贝，哪些不可以，现在只能把一些我觉的影响比较小的，先处理了
            */
            it = retrieve_orig_inst(b, it, 1);
            break;

        case CPUI_INT_MULT:
            if (istemp(p->output)) {
                if (p1->opcode == CPUI_INT_ADD) {
                    _mla(reg2i(poa(p1)), reg2i(pi0a(p)), reg2i(pi1a(p)), reg2i(pi1a(p1)));
                }
                else if (p1->opcode == CPUI_INT_SUB) {
                    _mls(reg2i(poa(p1)), reg2i(pi0a(p)), reg2i(pi1a(p)), reg2i(pi0a(p1)));
                }

                advance(it, 1);
            }
            else if (isreg(p->output)) {
                _mul(reg2i(poa(p)), reg2i(pi0a(p)), reg2i(pi1a(p)));
            }
            break;

        case CPUI_BOOL_NEGATE:
            p4 = *it1;
            if (p3->opcode == CPUI_CBRANCH) {
                if (p->opcode == CPUI_BOOL_NEGATE
                    && p1->opcode == CPUI_BOOL_AND
                    && p2->opcode == CPUI_BOOL_NEGATE
                    && pi0a(p) == azr
                    && pi0a(p1) == acy) {
                    write_cbranch(b, COND_LS);
                }
            }
            else if (p4->opcode == CPUI_CBRANCH) {
                if (p1->opcode == CPUI_INT_EQUAL
                    && p2->opcode == CPUI_BOOL_AND
                    && p3->opcode == CPUI_BOOL_NEGATE
                    && pi0a(p) == azr
                    && pi0a(p1) == ang
                    && pi1a(p1) == aov) {
                    write_cbranch(b, COND_LE);
                }
            }
            it = advance_to_inst_end(it);
            break;

        case CPUI_SUBPIECE:
            if (p1->opcode == CPUI_SUBPIECE) {
                if (p2->opcode == CPUI_INT_SEXT) {
                    it = retrieve_orig_inst(b, it, 1);
                }
            }
            break;

        default:
            break;
        }

inst_label:
        if (oind == ind)
            vm_error("thumbgen find un-support pcode seq[%d]", p->start.getTime());

        /* 打印新生成的代码 */
#if 1
        int len = ind - oind, siz;

        while (len > 0) {
            siz = dump_one_inst(oind, p);
            len -= siz;
            oind += siz;
        }
#endif
    }

    if (b->out.size() == 0) return 0;

    /* 相对跳转，不需要任何额外处理，直接返回 */
    if (b->is_rel_header()) return 0;

    int add_jmp = 0;
    flowblock *b1;

    if (b->out.size() == 1) {
        b1 = b->get_out(0);
        if (((b_ind + 1) == blist.size()) || (blist[b_ind + 1] != b1)) 
            add_jmp = 1;
    }
    else if (b->out.size() == 2) {
        assert(b->last_op()->opcode == CPUI_CBRANCH);
        b1 = b->get_false_edge()->point;
        if (((b_ind + 1) == blist.size()) || (blist[b_ind + 1] != b1)) 
            add_jmp = 1;
    }
    else 
        throw LowlevelError("now not support switch code gen");

    /* 假如出现了需要修复的vldr指令，那么我们把所有加载的数据全部移动到，这个block末尾，

    那么这个块末尾，必须得加上一个jmp，否则会把数据解析成指令


    b1:

    0002:   xxx 
    0004:   xxxx
    cbranch bX
    branch bN
    data: xxxx
    */

    if (fix_vlist.size()) 
        add_jmp = 1;

    if (add_jmp) {
        //x = COND_AL << 22;
        x = 0xf0009000;
        x |=  b1->cg.data ?  encbranch2(ind, b1->cg.data - data, 0):encbranch2(0, 0, 0);
        if (!b1->cg.data)
            add_fix_list(ind, b1, COND_AL);
        o(x);
        // dump_one_inst(ind - 4, NULL);
    }

    for (i = 0; i < fix_vlist.size(); i++)
        fix_vldr(fix_vlist[i]);

    return 0;
}

/* A8.8.333 */
void thumb_gen::fix_vldr(fix_vldr_item &vldr)
{
    uint1 fillbuf[16];
    int siz = vldr.end->output->get_size(), oind = ind, offset;


    offset = (oind - vldr.ind - 4);
    if (offset >= 1024)
        vm_error("vldr only support <1024 offset");

    /* 
    4字节对齐，因为vldr加载的偏移必须4字节对齐，假如偏移不够，则补齐， 因为thumb指令，
    只有2字节对齐，和4字节对齐2种，假如没有4字节对齐，那么就是2字节，对齐，补2个字节即可 
    */
    if (!align4(offset)) {
        ind += 2;
        offset += 2;
    }

    d->loader1->loadFill(fillbuf, siz, Address(d->get_code_space(), vldr.end->get_in(1)->get_val()));

    memcpy(data + ind, fillbuf, siz);
    ind += siz;

    uint32_t inst = read_thumb2(data + vldr.ind);

    inst &= ~0xff;

    inst |= (offset >> 2);

    write_thumb2(inst, data + vldr.ind);
}

void thumb_gen::write_cbranch(flowblock *b, uint32_t cond)
{
    uint32_t x;
    flowblock *t = b->get_true_edge()->point;
    x = (cond << 22);
    x |=  t->cg.data ?  encbranch(ind, t->cg.data - data, 0):encbranch(0, 0, 0);
    if (!t->cg.data)
        add_fix_list(ind, t, 0);
    o(x);
}

int thumb_gen::dump_one_inst(int oind, pcodeop *p)
{
    int i, siz;
    AssemblyRaw assem;
    assem.disable_html();
    char buf[128];
    assem.set_buf(buf);
    siz = d->trans->printAssembly(assem, Address(d->trans->getDefaultCodeSpace(), oind));

    if (p)
        printf("[p%4d] ", p->start.getTime());
    else
        printf("[     ] ");

    for (i = 0; i < siz; i++)
        printf("%02x ", data[oind + i]);

    for (; i < 4; i++)
        printf("   ");

    puts(buf);

    return siz;
}

void thumb_gen::add_fix_list(int ind, flowblock *b, int cond)
{
    flist.push_back(new fix_item(ind, b, cond));
}

void thumb_gen::dump()
{
    funcdata *fd1 = new funcdata(fd->name.c_str(), fd->get_addr(), 0, d);

    fd1->flags.dump_inst = 1;
    fd1->flags.thumb = fd->get_thumb();
    fd1->follow_flow();
    fd1->heritage();
    fd1->dump_cfg(fd1->name, "after_codegen", 1);
}

void thumb_gen::preprocess()
{
    int ret;
    pass_cond_reduce pass_cond_reduce(fd);
    pass_regalloc_const_arm pass_regalloc(fd);

    fd->flags.disable_to_const = 1;

    ret = pass_cond_reduce.run();
    if (PASS_DO_STHING(ret)) {
        fd->heritage_clear();
        fd->heritage();
    }
#if 0
    ret = pass_regalloc.run();
    if (PASS_DO_STHING(ret)) {
        fd->heritage_clear();
        fd->heritage();
    }
#endif
    fd->bblocks.compute_local_live_sets_p();
    fd->bblocks.compute_global_live_sets_p();
}

int thumb_gen::regalloc(pcodeop *p)
{
    int i;

    for (i = R0; i <= R12; i++) {
        if (!p->live_in.test(i)) {
            p->live_in.set(i);
            return i;
        }
    }

    vm_error("regalloc failure on pcode[%d]", p->start.getTime());
    return -1;
}

/* 
@save 写到so buf内
*/
pit thumb_gen::retrieve_orig_inst(flowblock *b, pit pit, int save)
{
    pcodeop *p, *prevp = *pit;

    for (++pit; pit != b->ops.end(); prevp = p, pit++) {
        p = *pit;
        if (p->get_addr() != prevp->get_addr()) break;
    }

    int size = fd->inst_size(prevp->get_addr());
    assert(size > 0);
    d->loader1->loadFill(fillbuf, size, prevp->get_addr());

    if (save)
        ob(fillbuf, size);

    return --pit;
}

pit thumb_gen::advance_to_inst_end(pit pit)
{
    flowblock *b = (*pit)->parent;
    const Address &addr = (*pit)->get_addr();

    for (++pit; pit != b->ops.end(); pit++) {
        if ((*pit)->get_addr() != addr)
            return --pit;
    }

    return --pit;
}

int thumb_gen::save_to_end(uint32_t imm)
{
    if ((end - 4) >= ind)
        vm_error("filesize need expand");

    end =- 4;
    mbytes_write_int_little_endian_4b(data + end, imm);

    return end;
}

pit thumb_gen::g_push(flowblock *b, pit pit)
{
    int reglist = 0;

    pcodeop *p = *pit, *p1;

    if ((p->opcode == CPUI_COPY) && pi0a(p) == asp) 
        p = *++pit;

    while (p->output->get_addr() != d->sp_addr) {
        p = *pit++;
        p1 = *pit++;
        if ((p->opcode == CPUI_INT_SUB) && (p1->opcode == CPUI_STORE) && (pi1a(p1) == ama)) {
            reglist |= 1 << reg2i(pi2a(p1));
        }
        else throw LowlevelError("not support");

        p = *pit;
    }

    if ((poa(p) == asp) && (pi0a(p) == ama)) {
        if ((p->opcode == CPUI_COPY)
            /* FIXME: rn = rm + 0 == > rn = copy rm*/
            || ((p->opcode == CPUI_INT_ADD) && pi1(p)->is_constant())
            || ((p->opcode == CPUI_INT_SUB) && pi1(p)->is_constant())) {
            /* NOTE:
            所有的push {reglist} 指令，在转成pcode以后，末尾对sp的操作都是等于copy的，一般有以下几种形式:

            1. sp = COPY mult_addr
            2. mult_addr = INT_SUB mult_addr, 4:4 
               sp = INT_ADD mult_addr, 4:4

            第2种情况，可以合并成 sp = copy multi_addr，大家认真看，上面的 -4 和 +4 可以合并掉

            所以我们在把pcode转回push时，一般搜索到store的后一条指令为止，也就是 sp = copy multi_addr为止，
            但是可能出现这样一种情况

            1. sp = copy multi_addr
            2  sp = add sp, 4

            在某些情况下指令1， 2 合并成了 sp = add multi_addr, 4

            针对这种情况，当我们发现store的最后一条指令不是等价的sp = copy multi_addr时，要退回pit指针
            */
            if ((p->opcode != CPUI_COPY) && pi1(p)->get_val())
                pit--;
            _push(reglist);
        }
    }

    return pit;
}

pit thumb_gen::g_pop(flowblock *b, pit pit)
{
    int reglist = 0;
    pcodeop *p = *pit, *p1;

    p = *pit++;
    p1 = *pit++;
    reglist |= 1 << reg2i(poa(p1));

    while (1) {
        p = *pit++;
        p1 = *pit++;
        if ((p->opcode == CPUI_INT_ADD) && (p1->opcode == CPUI_LOAD)) {
            reglist |= 1 << reg2i(poa(p1));
        }
        else
            break;
    }

    if ((p1->opcode == CPUI_COPY)) {
        _pop(reglist);

        if ((reglist & (1 << PC))) {
            if ((*pit++)->opcode != CPUI_INT_AND
                || (*pit)->opcode != CPUI_RETURN)
                vm_error("pop need pc operation ");
        }

        return pit;
    }
    vm_error("pop meet error pcode, %d", p->start.getTime());
    return pit;
}

pit thumb_gen::g_vpush(flowblock *b, pit pit)
{
    pcodeop *p = *pit, *p1;
    uint32_t reglist = 0, size = 0, dsize = 0, single = 0, x;

    assert(p->opcode == CPUI_INT_SUB);
    size = p->get_in(1)->get_val();
    p = *++pit;

    while (poa(p) == ama) {
        p = *pit++;
        p1 = *pit++;
        if (((p->opcode == CPUI_INT_ADD) || (p->opcode == CPUI_COPY)) && (p1->opcode == CPUI_STORE) && (pi1a(p1) == ama)) {
            single = p1->get_in(2)->get_size() == 4;
            dsize += p1->get_in(2)->get_size();
            x = d->vreg2i(pi2a(p1));
            if (single) reglist |= 1 << x;
            else 
                reglist |= (1 << x) + (1 << (x + 1));
        }
        else throw LowlevelError("not support");

        p = *pit;
    }

    _sub_sp_imm(size - dsize);
    _vpush(single, reglist);

    return pit;
}


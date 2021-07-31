/* Unicorn Emulator Engine */
/* By Nguyen Anh Quynh, 2015 */

/* Sample code to demonstrate how to emulate ARM code */

#include <unicorn/unicorn.h>
#include <string.h>
#include "mcore/mcore.h"
#include "uc_util.h"

int arm_general_regs[16] = {
    UC_ARM_REG_R0,
    UC_ARM_REG_R1,
    UC_ARM_REG_R2,
    UC_ARM_REG_R3,
    UC_ARM_REG_R4,
    UC_ARM_REG_R5,
    UC_ARM_REG_R6,
    UC_ARM_REG_R7,
    UC_ARM_REG_R8,
    UC_ARM_REG_R9,
    UC_ARM_REG_R10,
    UC_ARM_REG_R11,
    UC_ARM_REG_R12,
    UC_ARM_REG_R13,
    UC_ARM_REG_R14,
    UC_ARM_REG_PC
};

static void dump_regs(uc_engine *uc);

struct base64_test {
    int regs[count_of_array(arm_general_regs)];
    struct uc_runtime *ur;
};

void base64_test_on_main_exit()
{
}

int uc_reg_read_batch2(uc_engine *uc, int *ids, int *vals, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        uc_reg_read(uc, ids[i], vals + i);
    }

    return 0;
}

void base64_test_on_exit(struct base64_test *test)
{
    struct uc_runtime *ur = test->ur;
    int r0;
    char buf[128];

    uc_emu_stop(ur->uc);

    uc_reg_read(ur->uc, UC_ARM_REG_R0,  &r0);

    uc_mem_read(ur->uc, test->regs[1], buf, sizeof (buf));
    printf("base64_encode = %s, size = %d\n", buf, r0);
}

int base64_test_init(struct base64_test *test, struct uc_runtime *ur)
{
    int sp, r0, r1;
    uc_engine *uc = ur->uc;
    struct uc_hook_func *f;

    test->ur = ur;


    sp = (int)ur_stack_end(ur) + 1;
    uc_reg_write(uc, UC_ARM_REG_SP, &sp);

    r0 = ur_string32(ur, "hello, world");
    uc_reg_write(uc, UC_ARM_REG_R0, &r0);

    r1 = (int)ur_malloc(ur, 100);
    uc_reg_write(uc, UC_ARM_REG_R1, &r1);

    f = ur_alloc_func(ur, "main_exit", base64_test_on_exit, test);
    uc_reg_write(uc, UC_ARM_REG_LR, &f->address);

    uc_reg_read_batch2(ur->uc, arm_general_regs, test->regs, count_of_array(test->regs));

    dump_regs(uc);

    printf("start run====================================");

    return 0;
}

static void dump_regs(uc_engine *uc)
{
    int regs[16];

    uc_reg_read_batch2(uc, arm_general_regs, regs, 16);

    printf("\tr0=%08x, r1=%08x, r2=%08x,   r3=%08x,  r4=%08x, r5=%08x, r6=%08x, r7=%08x\n"
           "\tr8=%08x, r9=%08x, r10=%08x, r11=%08x, r12=%08x, sp=%08x, lr=%08x, pc=%08x\n",
        regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7],
        regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
}

static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    printf(">>> Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", address, size);
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    struct uc_runtime *t = user_data;
    struct uc_hook_func *hook;

    int i;
    printf("%llx ", address);

    uint8_t buf[128];

    uc_mem_read(uc, address, buf, size);

    for (i = 0; i < (int)size; i++)
        printf("%02x ", buf[i]);
    printf("\n");

    dump_regs(uc);

    printf("\n");

    if ((hook = ur_hook_func_find_by_addr(t, address))) {
        if (hook->cb) {
            hook->cb(hook->user_data);
        }
    }
}

static void main_exit(struct uc_runtime *ur, void *user_data)
{
    uc_emu_stop(ur->uc);
}


#define MEM_MAP_SIZE        (2 * MB)

static void thumb_test_base64(const char *soname)
{
    uc_engine *uc;
    uc_err err;
    uc_hook trace1, trace2;

    // Initialize emulator in ARM mode
    err = uc_open(UC_ARCH_ARM, UC_MODE_THUMB, &uc);
    if (err) {
        printf("Failed on uc_open() with error returned: %u (%s)\n",
                err, uc_strerror(err));
        return;
    }

    uc_runtime_t *ur = uc_runtime_new(uc, soname, 0, 0);
    if (!ur)
        return;

    struct base64_test test;

    base64_test_init(&test, ur);

    uc_hook_add(uc, &trace1, UC_HOOK_BLOCK, hook_block, ur, 1, 0);
    uc_hook_add(uc, &trace2, UC_HOOK_CODE, hook_code, ur, ur_text_start(ur), ur_text_end(ur));

    uint64_t start = ur_symbol_addr(ur, "base64_encode");
    err = uc_emu_start(uc, start, 0, 0, 0);
    if (err) {
        printf("Failed on uc_emu_start() with error returned: %s(%d)\n", uc_strerror(err), err);
    }

    // now print out some registers
    printf(">>> Emulation done. Below is the CPU context\n");

    dump_regs(uc);

    uc_close(uc);
}





static const char *help = {
    "test1 [data_dir] \n"
};

int main(int argc, char **argv, char **envp)
{
    // dynamically load shared library
#ifdef DYNLOAD
    if (!uc_dyn_load(NULL, 0)) {
        printf("Error dynamically loading shared library.\n");
        printf("Please check that unicorn.dll/unicorn.so is available as well as\n");
        printf("any other dependent dll/so files.\n");
        printf("The easiest way is to place them in the same directory as this app.\n");
        return 1;
    }
#endif

#if 1
    char buf[128];
    if (argc != 2) {
        puts(help);
        return -1;
    }

    //test_thumb2();

    sprintf(buf, "%s/unittests/base64/libs/armeabi-v7a/libbase64.so", argv[1]);
    thumb_test_base64(buf);
#else
    test_arm();
    test_thumb();
    test_thumb_ite();
#endif

    // dynamically free shared library
#ifdef DYNLOAD
    uc_dyn_free();
#endif

    return 0;
}

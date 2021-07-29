/* Unicorn Emulator Engine */
/* By Nguyen Anh Quynh, 2015 */

/* Sample code to demonstrate how to emulate ARM code */

#include <unicorn/unicorn.h>
#include <string.h>
#include "mcore/mcore.h"
#include "uc_util.h"


// code to be emulated
#define ARM_CODE "\x37\x00\xa0\xe3\x03\x10\x42\xe0" // mov r0, #0x37; sub r1, r2, r3
#define THUMB_CODE "\x83\xb0" // sub    sp, #0xc
#define THUMB_CODE2 "\xf0\xb5"


#define ARM_THUM_COND_CODE "\x9a\x42\x14\xbf\x68\x22\x4d\x22" // 'cmp r2, r3\nit ne\nmov r2, #0x68\nmov r2, #0x4d'

// memory address where emulation starts
#define ADDRESS 0x10000

static void dump_regs(uc_engine *uc)
{
    int sp, r0, r1, pc;

    uc_reg_read(uc, UC_ARM_REG_SP, &sp);
    uc_reg_read(uc, UC_ARM_REG_R0, &r0);
    uc_reg_read(uc, UC_ARM_REG_R1, &r1);
    uc_reg_read(uc, UC_ARM_REG_PC, &pc);

    printf("\tsp=%08x, r0=%08x, r1=%08x, pc=%08x\n", sp, r0, r1, pc);
}

static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    printf(">>> Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", address, size);
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    int i;
    printf("%llx ", address);

    uint8_t buf[128];

    uc_mem_read(uc, address, buf, size);

    for (i = 0; i < (int)size; i++)
        printf("%02x ", buf[i]);
    printf("\n");

    dump_regs(uc);

    printf("\n");

    if (ur_hook_func_find_by_addr(user_data, address)) {
    }
}

#define MEM_MAP_SIZE        (2 * MB)

static void thumb_test_base64(const char *soname)
{
    uc_engine *uc;
    uc_err err;
    uc_hook trace1, trace2;

    int sp, r0, r1;

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

    sp = (int)ur_stack_end(ur) + 1;
    uc_reg_write(uc, UC_ARM_REG_SP, &sp);

    r0 = ur_string32(ur, "hello, world");
    uc_reg_write(uc, UC_ARM_REG_R0, &r0);

    r1 = (int)ur_malloc(ur, 100);
    uc_reg_write(uc, UC_ARM_REG_R1, &r1);

    uc_hook_add(uc, &trace1, UC_HOOK_BLOCK, hook_block, ur, 1, 0);
    uc_hook_add(uc, &trace2, UC_HOOK_CODE, hook_code, ur, ur_text_start(ur), ur_text_end(ur));

    uint64_t start = ur_symbol_addr(ur, "base64_encode");
    err = uc_emu_start(uc, start, 0, 0, 0);
    if (err) {
        printf("Failed on uc_emu_start() with error returned: %s(%d)\n", uc_strerror(err), err);
    }

    // now print out some registers
    printf(">>> Emulation done. Below is the CPU context\n");

    uc_reg_read(uc, UC_ARM_REG_SP, &sp);

    printf(">>> SP = 0x%x\n", sp);

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

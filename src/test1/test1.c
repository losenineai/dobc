/* Unicorn Emulator Engine */
/* By Nguyen Anh Quynh, 2015 */

/* Sample code to demonstrate how to emulate ARM code */

#include <unicorn/unicorn.h>
#include "mcore/mcore.h"
#include "uc_util.h"
#include "test.h"
#include "test_base64.h"

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

struct base64_test {
    int regs[count_of_array(arm_general_regs)];
    struct uc_runtime *ur;
};

static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    printf(">>> Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", address, size);
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    struct uc_runtime *t = user_data;
    struct uc_hook_func *hook;

    int i;

    if (t->debug.trace) {
        test_dump_regs(t);

        printf("%llx ", address);

        uint8_t buf[128];

        uc_mem_read(uc, address, buf, size);

        for (i = 0; i < (int)size; i++)
            printf("%02x ", buf[i]);
        printf("\n");
    }

    if ((hook = ur_hook_func_find_by_addr(t, address))) {
        if (hook->cb) {
            hook->cb(hook->user_data);
        }
    }
}

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

    ur->debug.trace = 1;

    test_base64_encode_init1(ur);

    uc_hook_add(uc, &trace1, UC_HOOK_BLOCK, hook_block, ur, 1, 0);
    uc_hook_add(uc, &trace2, UC_HOOK_CODE, hook_code, ur, ur_text_start(ur), ur_text_end(ur));

    uint64_t start = ur_symbol_addr(ur, "base64_encode");
    err = uc_emu_start(uc, start, 0, 0, 0);
    if (err) {
        printf("Failed on uc_emu_start() with error returned: %s(%d)\n", uc_strerror(err), err);
    }

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

    char buf[128];
    if (argc != 2) {
        puts(help);
        return -1;
    }

    //test_thumb2();

    //sprintf(buf, "%s/unittests/base64/libs/armeabi-v7a/libbase64.so", argv[1]);
    sprintf(buf, "%s/unittests/base64/libs/armeabi-v7a/libbase64.so.decode", argv[1]);
    thumb_test_base64(buf);

    // dynamically free shared library
#ifdef DYNLOAD
    uc_dyn_free();
#endif

    return 0;
}

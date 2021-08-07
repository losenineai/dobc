
#include "test.h"

static int arm_general_regs[16] = {
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

int test_dump_regs(uc_runtime_t *r)
{
    int regs[16];

    ur_reg_read_batch(r, arm_general_regs, regs, 16);

    printf("\tr0=%08x, r1=%08x, r2=%08x,   r3=%08x,  r4=%08x, r5=%08x, r6=%08x, r7=%08x\n"
           "\tr8=%08x, r9=%08x, r10=%08x, r11=%08x, r12=%08x, sp=%08x, lr=%08x, pc=%08x\n",
        regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7],
        regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);

    return 0;
}

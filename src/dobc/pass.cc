
#include "pass.hh"

int pass_cond_reduce::run()
{
    return 0;
}

int pass_regalloc_const_arm::run()
{
    fd->bblocks.compute_local_live_sets();
    fd->bblocks.compute_global_live_sets();
    fd->bblocks.dump_live_sets();

    return 0;
}


#include "vm.h"
#include "dobc.hh"

#define PSPEC_FILE          "../../../Processors/ARM/data/languages/ARMCortex.pspec"
#define CSPEC_FILE          "../../../Processors/ARM/data/languages/ARM.cspec"
#define TEST_SO             "../../../data/vmp/360_1/libjiagu.so"

static char help[] = {
    "dobc [-i filename] \r\n" 
    "       -o                      output filename\r\n"
    "       -t                      ollvm | 360freevmp, default is ollvm"
    "       -sd                     dump new so to so directory or current directory\r\n"
    "       -da [hex address, ]     decode address list\r\n"
    "       --ghidra                ghidra config directory\r\n"
    "\n\n"
    "       -debug.cfg              dump static trace cfg\r\n"
    "       -debug.level [0-6]      debug info level, [error -> detail]\r\n"
    "       -debug.inst0            dump stage0(dobc.initial) process instruction \r\n"
    "       -debug.inst1            dump stage1(dobc.codegen-ing, not fix pc-relative instruction) process instruction \r\n"
    "       -debug.inst2            dump stage2(dobc.codegen-success) process instruction \r\n"
    "       -debug.zxw              this option only used by author(zhengxianwei) \r\n"
};

int main(int argc, char **argv)
{
    int i, sd = 0; 
    dobc d;
    const char *filename;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-st")) {
            d.set_shelltype(argv[++i]);
        }
        else if (!strcmp(argv[i], "-i")) {
            filename = argv[++i];
            d.set_input_bin(filename);
        }
        else if (!strcmp(argv[i], "-o")) {
            d.set_output_filename(argv[++i]);
        }
        else if (!strcmp(argv[i], "-sd")) {
            sd = 1;
        }
        else if (!strcmp(argv[i], "-c")) {
            d.set_ghidra(argv[++i]);
        }
        else if (!strcmp(argv[i], "-debug.level")) {
            print_level_set(atoi(argv[++i]));
        }
        else if (!strcmp(argv[i], "-da")) {
            while (((i+ 1) < argc) && argv[i + 1][0] != '-') {
                d.decode_address_list.push_back(strtoul(argv[i + 1], NULL, 16));
                i++;
            }
        }
        else if (!strcmp(argv[i], "-debug.cfg")) {
            d.debug.dump_cfg = 1;
        }
        else if (!strcmp(argv[i], "-debug.zxw")) {
            print_level_set(PRINT_LEVEL_INFO);

            d.debug.dump_inst0 = 1;
            d.debug.dump_inst1 = 1;
            d.debug.dump_inst2 = 1;
        }
    }

    if (d.ghidra.empty() || d.filename.empty() || d.decode_address_list.empty()) {
        puts(help);
        return -1;
    }

    if (sd)
        d.out_dir.assign(filename, basename(filename) - filename);

    DocumentStorage docstorage;
    Element *sleighroot = docstorage.openDocument(d.slafilename)->getRoot();
    docstorage.registerTag(sleighroot);

    d.init(docstorage);

    d.run();

    return 0;
}

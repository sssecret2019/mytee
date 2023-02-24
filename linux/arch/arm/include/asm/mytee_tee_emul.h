#ifndef __ASSEMBLY__
#include <linux/types.h>

extern u8 mytee_tee_emul_enabled;

#define mytee_tee_write_sysreg(v, r) do {                \
    u32 __val = (u32)(v);                    \
    asm volatile(                        \
    "stmfd sp!, {r0, r1}\n"                \
    "mov r1, %0\n"                        \
    "mov r0, %1\n"                        \
    "smc #1\n"                        \
    "ldmfd sp!, {r0, r1}"                \
    :: "r"(__val), "I"(r));                \
    } while(0);


enum {
    EMUL_TTBR0 = 1,
    EMUL_SCTLR,
    EMUL_TTBR1,
    EMUL_TCR,
    EMUL_MAIR,
    EMUL_VBAR,
    EMUL_MEM    
};

int mytee_tee_mem_emul(u32 command, u32 dest, u32 val_h, u32 val_l);

#else

#define EMUL_TTBR0     1
#define EMUL_SCTLR     2
#define EMUL_TTBR1     3
#define EMUL_TCR    4
#define EMUL_MAIR    5
#define    EMUL_VBAR    6    
#define EMUL_MEM    7

    .macro mytee_tee_emul_priv_inst, emtype, param1, param2
    .arch_extension sec
    // Save r0 - r1
    stmfd sp!, {r0-r2}
    // x1: contains the value
    mov r1, \param1
    mov r2, \param2
    // x0 contains the emulation type
    mov r0, #\emtype
    smc #1
//    msr     ttbr0_el1, x1
    // Restore x0 - x1
    ldmfd sp!, {r0-r2}
    .endm
#endif


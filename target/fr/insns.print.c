#include "qemu/osdep.h"
#include "disas/dis-asm.h"
#include "cpu.h"
#include "exec/log.h"

typedef struct DisasContext {
    bfd_byte    *buffer;
    int         len;
    uint32_t    imm;
    bfd_vma     pc;
} DisasContext;

static inline uint32_t ex_load_32(DisasContext *dc)
{
    dc->len += 4;
    dc->imm = bfd_getb32(dc->buffer+2);
    return dc->imm;
}

static inline uint32_t ex_load_20(DisasContext *dc, int n)
{
    dc->len += 2;
    dc->imm = (bfd_getb16(dc->buffer+2)>>16);
    return dc->imm|(n<<16);
}

static inline uint32_t ex_load_u4c(DisasContext *dc)
{
    dc->len += 2;
    dc->imm = bfd_getb16(dc->buffer+2)>>16;
    return dc->imm;
}

#define ex_lshift_2(dc, n)  ((n)<<2)
#define ex_lshift_1(dc, n)  ((n)<<1)

#include "decode.inc.c"

int do_print_insn_fr(bfd_vma memaddr, bfd_byte *buffer)
{
    uint16_t insn;
    DisasContext dc = {
        .buffer = buffer,
        .len = 2,
        .imm = 0,
        .pc = memaddr
    };

    insn = (bfd_getb16(buffer) >> 16);
    qemu_log("%04x", insn);

    fr_decode(&dc, insn);

    return dc.len;
}

#define FR_GEN_INSN_PRINT(insn, fmt, arg...)                            \
static bool glue(trans_, insn)(DisasContext *dc, glue(arg_, insn) *a)   \
{                                                                       \
    if (dc->imm) {                                                      \
        qemu_log(" %8.*x\t", (dc->len-2)*2, dc->imm);                   \
    } else {                                                            \
        qemu_log(" %8s\t", "");                                         \
    }                                                                   \
    qemu_log(fmt, ##arg);                                               \
    return true;                                                        \
}

static const char * const regnames[] = {
    "R0",         "R1",         "R2",         "R3",
    "R4",         "R5",         "R6",         "R7",
    "R8",         "R9",         "R10",        "R11",
    "R12",        "AC",         "FP",         "SP",
    "TBR",        "RP",         "SSP",        "USP",
    "MDL",        "MDH",        "rsrv",       "rsrv",
    "rsrv",       "rsrv",       "rsrv",       "rsrv",
    "rsrv",       "rsrv",       "rsrv",       "rsrv",
    "PC",         "PS"
};

#define Ri      regnames[a->ri]
#define Rj      regnames[a->rj]
#define Rs      regnames[a->rs+DR_BASE]

FR_GEN_INSN_PRINT(ADD, "ADD %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(ADD2, "ADD #%xH, %s", a->s5, Ri)
FR_GEN_INSN_PRINT(ADDC, "ADDC %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(ADDN, "ADDN %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(ADDN2, "ADDN #%xH, %s", a->s5, Ri)
FR_GEN_INSN_PRINT(SUB, "SUB %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(SUBC, "SUBC %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(SUBN, "SUBN %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(CMP, "CMP %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(CMP2, "CMP #%xH, %s", a->s5, Ri)
FR_GEN_INSN_PRINT(AND, "AND %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(AND1, "AND %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(ANDH, "ANDH %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(ANDB, "ANDB %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(OR, "OR %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(OR1, "OR %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(ORH, "ORH %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(ORB, "ORB %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(EOR, "EOR %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(EOR1, "EOR %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(EORH, "EORH %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(EORB, "EORB %s, @%s", Rj, Ri)
FR_GEN_INSN_PRINT(BANDL, "BANDL #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BANDH, "BANDH #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BORL, "BORL #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BORH, "BORH #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BEORL, "BEORL #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BEORH, "BEORH #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BTSTL, "BTSTL #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(BTSTH, "BTSTH #%xH, @%s", a->u4, Ri)
FR_GEN_INSN_PRINT(MUL, "MUL %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(MULU, "MULU %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(MULH, "MULH %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(MULUH, "MULUH %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(DIV0S, "DIV0S %s", Ri)
FR_GEN_INSN_PRINT(DIV0U, "DIV0U %s", Ri)
FR_GEN_INSN_PRINT(DIV1, "DIV1 %s", Ri)
FR_GEN_INSN_PRINT(DIV2, "DIV2 %s", Ri)
FR_GEN_INSN_PRINT(DIV3, "DIV3")
FR_GEN_INSN_PRINT(DIV4S, "DIV4S")
FR_GEN_INSN_PRINT(LSL, "LSL %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(LSL2, "LSL #%xH, %s", a->u5, Ri)
FR_GEN_INSN_PRINT(LSR, "LSR %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(LSR2, "LSR #%xH, %s", a->u5, Ri)
FR_GEN_INSN_PRINT(ASR, "ASR %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(ASR2, "ASR #%xH, %s", a->u5, Ri)
FR_GEN_INSN_PRINT(LDI32, "LDI:32 #%xH, %s", a->i32, Ri)
FR_GEN_INSN_PRINT(LDI20, "LDI:20 #%xH, %s", a->i20, Ri)
FR_GEN_INSN_PRINT(LDI8, "LDI:8 #%xH, %s", a->i8, Ri)
FR_GEN_INSN_PRINT(LD, "LD @%s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(LD1, "LD @(R13, %s), %s", Rj, Ri)
FR_GEN_INSN_PRINT(LD2, "LD @(R14, %xH), %s", a->disp10, Ri)
FR_GEN_INSN_PRINT(LD3, "LD @(R15, %xH), %s", a->udisp6, Ri)
FR_GEN_INSN_PRINT(LD4, "LD @R15+, %s", Ri)
FR_GEN_INSN_PRINT(LD5, "LD @R15+, %s", Rs)
FR_GEN_INSN_PRINT(LD6, "LD @R15+, PS")
FR_GEN_INSN_PRINT(LDUH, "LDUH @%s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(LDUH1, "LDUH @(R13, %s), %s", Rj, Ri)
FR_GEN_INSN_PRINT(LDUH2, "LDUH @(R14, %xH), %s", a->disp9, Ri)
FR_GEN_INSN_PRINT(LDUB, "LDUB @%s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(LDUB1, "LDUB @(R13, %s), %s", Rj, Ri)
FR_GEN_INSN_PRINT(LDUB2, "LDUB @(R14, %xH), %s", a->disp8, Ri)
FR_GEN_INSN_PRINT(ST, "ST %s, @%s", Ri, Rj)
FR_GEN_INSN_PRINT(ST1, "ST %s, @(R13, %s)", Ri, Rj)
FR_GEN_INSN_PRINT(ST2, "ST %s, @(R14, %xH)", Ri, a->disp10)
FR_GEN_INSN_PRINT(ST3, "ST %s, @(R15, %xH)", Ri, a->udisp6)
FR_GEN_INSN_PRINT(ST4, "ST %s, @-R15", Ri)
FR_GEN_INSN_PRINT(ST5, "ST %s, @-R15", Rs)
FR_GEN_INSN_PRINT(ST6, "ST PS, @-R15")
FR_GEN_INSN_PRINT(STH, "STH %s, @%s", Ri, Rj)
FR_GEN_INSN_PRINT(STH1, "STH %s, @(R13, %s)", Ri, Rj)
FR_GEN_INSN_PRINT(STH2, "STH %s, @(R14, %xH)", Ri, a->disp9)
FR_GEN_INSN_PRINT(STB, "STB %s, @%s", Ri, Rj)
FR_GEN_INSN_PRINT(STB1, "STB %s, @(R13, %s)", Ri, Rj)
FR_GEN_INSN_PRINT(STB2, "STB %s, @(R14, %xH)", Ri, a->disp8)
FR_GEN_INSN_PRINT(MOV, "MOV %s, %s", Rj, Ri)
FR_GEN_INSN_PRINT(MOV1, "MOV %s, %s", Rs, Ri)
FR_GEN_INSN_PRINT(MOV2, "MOV PS, %s", Ri)
FR_GEN_INSN_PRINT(MOV3, "MOV %s, %s", Ri, Rs)
FR_GEN_INSN_PRINT(MOV4, "MOV %s, PS", Ri)
FR_GEN_INSN_PRINT(JMP, "JMP @%s", Ri)
FR_GEN_INSN_PRINT(CALL, "CALL %"PRIx64"H", a->label12+dc->pc+2)
FR_GEN_INSN_PRINT(CALL1, "CALL @%s", Ri)
FR_GEN_INSN_PRINT(RET, "RET")
FR_GEN_INSN_PRINT(INT, "INT #%xH", a->u8)
FR_GEN_INSN_PRINT(INTE, "INTE")
FR_GEN_INSN_PRINT(RETI, "RETI")
FR_GEN_INSN_PRINT(Bcc, "Bcc %"PRIx64"H", a->label9+dc->pc+2)
FR_GEN_INSN_PRINT(JMPD, "JMP:D @%s", Ri)
FR_GEN_INSN_PRINT(CALLD, "CALL:D %"PRIx64"H", a->label12+dc->pc+2)
FR_GEN_INSN_PRINT(CALLD1, "CALL:D @%s", Ri)
FR_GEN_INSN_PRINT(RETD, "RET:D")
FR_GEN_INSN_PRINT(BccD, "Bcc:D %"PRIx64"H", a->label9+dc->pc+2)
FR_GEN_INSN_PRINT(DMOV, "DMOV @%xH, R13", a->dir10)
FR_GEN_INSN_PRINT(DMOV1, "DMOV R13, @%xH", a->dir10)
FR_GEN_INSN_PRINT(DMOV2, "DMOV @%xH, @R13+", a->dir10)
FR_GEN_INSN_PRINT(DMOV3, "DMOV @R13+, @%xH", a->dir10)
FR_GEN_INSN_PRINT(DMOV4, "DMOV @%xH, @-R15", a->dir10)
FR_GEN_INSN_PRINT(DMOV5, "DMOV @R15+, @%xH", a->dir10)
FR_GEN_INSN_PRINT(DMOVH, "DMOVH @%xH, R13", a->dir9)
FR_GEN_INSN_PRINT(DMOVH1, "DMOVH R13, @%xH", a->dir9)
FR_GEN_INSN_PRINT(DMOVH2, "DMOVH @%xH, @R13+", a->dir9)
FR_GEN_INSN_PRINT(DMOVH3, "DMOVH @R13+, @%xH", a->dir9)
FR_GEN_INSN_PRINT(DMOVB, "DMOVB @%xH, R13", a->dir8)
FR_GEN_INSN_PRINT(DMOVB1, "DMOVB R13, @%xH", a->dir8)
FR_GEN_INSN_PRINT(DMOVB2, "DMOVB @%xH, @R13+", a->dir8)
FR_GEN_INSN_PRINT(DMOVB3, "DMOVB @R13+, @%xH", a->dir8)
FR_GEN_INSN_PRINT(COP, "LDRES/STRES")
FR_GEN_INSN_PRINT(COP1, "COPOP/COPLD/COPST/COPSV")
FR_GEN_INSN_PRINT(SRCH0, "SRCH0 %s", Ri)
FR_GEN_INSN_PRINT(SRCH1, "SRCH1 %s", Ri)
FR_GEN_INSN_PRINT(SRCHC, "SRCHC %s", Ri)
FR_GEN_INSN_PRINT(NOP, "NOP")
FR_GEN_INSN_PRINT(ANDCCR, "ANDCCR #%xH", a->u8)
FR_GEN_INSN_PRINT(ORCCR, "ORCCR #%xH", a->u8)
FR_GEN_INSN_PRINT(STILM, "STILM #%xH", a->u8)
FR_GEN_INSN_PRINT(ADDSP, "ADDSP #%xH", a->s10)
FR_GEN_INSN_PRINT(EXTSB, "EXTSB %s", Ri)
FR_GEN_INSN_PRINT(EXTUB, "EXTUB %s", Ri)
FR_GEN_INSN_PRINT(EXTSH, "EXTSH %s", Ri)
FR_GEN_INSN_PRINT(EXTUH, "EXTUH %s", Ri)
FR_GEN_INSN_PRINT(ENTER, "ENTER %xH", a->u10)
FR_GEN_INSN_PRINT(LEAVE, "LEAVE")
FR_GEN_INSN_PRINT(XCHB, "XCHB @%s, %s", Rj, Ri)

//FR_GEN_INSN_PRINT(LDM0, "LDM0 (reglist)")
//FR_GEN_INSN_PRINT(LDM1, "LDM1 (reglist)")
//FR_GEN_INSN_PRINT(STM0, "STM0 (reglist)")
//FR_GEN_INSN_PRINT(STM1, "STM1 (reglist)")

static bool trans_LSDM(DisasContext *dc, arg_LDM0 *a, bool is_ldm, bool is_r8r15)
{
    int i, reg, bit;
    DECLARE_BITMAP(ldm, 8) = {a->reglist};
    char buff[32] = "";
    int len;
    if (a->reglist == 0) return true;
    is_r8r15 = !!is_r8r15;

    qemu_log(" %8s\t", "");

    for (i=0; i<=7; i++) {
        if (is_ldm) {
            bit = i;
        } else {
            bit = 7-i;
        }
        reg = i + 8*is_r8r15;

        if (test_bit(bit, ldm)) {
            char Rn[8] = "";
            snprintf(Rn, sizeof(Rn), "R%d,", reg);
            strncat(buff, Rn, sizeof(buff)-1);
        }
    }
    len = strlen(buff);
    if (len != 0) buff[len-1] = '\0'; // remove the last ','

    qemu_log("%s%d (%s)", is_ldm?"LDM":"STM", is_r8r15, buff);
    return true;
}

static bool trans_LDM0(DisasContext *dc, arg_LDM0 *a)
{
    return trans_LSDM(dc, a, 1, 0);
}

static bool trans_LDM1(DisasContext *dc, arg_LDM1 *a)
{
    return trans_LSDM(dc, a, 1, 1);
}

static bool trans_STM0(DisasContext *dc, arg_STM0 *a)
{
    return trans_LSDM(dc, a, 0, 0);
}

static bool trans_STM1(DisasContext *dc, arg_STM1 *a)
{
    return trans_LSDM(dc, a, 0, 1);
}
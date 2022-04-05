
#define BITS_WORD       32
#define MSB_BIT_WORD    (BITS_WORD-1)
#define BITS_HALFWORD   16
#define BITS_BYTE       8

static void tcg_gen_flag_nz(TCGv ret, uint32_t bits)
{
    tcg_gen_shri_tl(cpu_PS_N, ret, bits-1);
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, ret, 0);
}

static void tcg_gen_flag_nz_2(TCGv ret)
{
    tcg_gen_movi_tl(cpu_PS_N, 0);
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, ret, 0);
}

static void tcg_gen_add_flag_v(TCGv ret, TCGv r1, TCGv r2)
{
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_xor_tl(cpu_PS_V, ret, r1);
    tcg_gen_xor_tl(tmp, r1, r2);
    tcg_gen_andc_tl(cpu_PS_V, cpu_PS_V, tmp);
    tcg_gen_shri_tl(cpu_PS_V, cpu_PS_V, MSB_BIT_WORD);
    tcg_temp_free(tmp);
}

static void tcg_gen_sub_flag_v(TCGv ret, TCGv r1, TCGv r2)
{
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_xor_tl(cpu_PS_V, ret, r1);
    tcg_gen_xor_tl(tmp, r1, r2);
    tcg_gen_and_tl(cpu_PS_V, cpu_PS_V, tmp);
    tcg_gen_shri_tl(cpu_PS_V, cpu_PS_V, MSB_BIT_WORD);
    tcg_temp_free(tmp);
}

static inline void GEN_CODE_ADD_t(TCGv r1, TCGv r2, bool carry)
{
    TCGv t0 = tcg_const_local_tl(0);
    TCGv tmp = tcg_temp_local_new();
    if (carry) {
        tcg_gen_add2_tl(tmp, cpu_PS_C, (r1), t0, cpu_PS_C, t0);
        tcg_gen_add2_tl(tmp, cpu_PS_C, (tmp), cpu_PS_C, (r2), t0);
    } else {
        tcg_gen_add2_tl(tmp, cpu_PS_C, (r1), t0, (r2), t0);
    }
    tcg_gen_flag_nz(tmp, BITS_WORD);
    tcg_gen_add_flag_v(tmp, (r1), (r2));
    tcg_gen_mov_tl((r1), tmp);
    tcg_temp_free(t0);
    tcg_temp_free(tmp);
}

#define WRITTING_R15(reg) do {if ((reg) == R_15) { flag |= FLAG_WRITE_R15;} }while(0)
#define WRITTING_SP(reg) do {if(((reg) == R_SSP) || ((reg)==R_USP)) { flag |= FLAG_WRITE_SP;} }while(0)

FR_GEN_CODE(ADD, "ADD Rj, Ri", "1", FLAG_UPDATE_NZVC,
({
    GEN_CODE_ADD_t(tcg_Ri, tcg_Rj, 0);
    WRITTING_R15(a->ri);
}))

#if 0  //merged to ADD2
FR_GEN_CODE(ADD1, "ADD #i4, Ri", "1", FLAG_UPDATE_NZVC,
({
    TCGv ti = tcg_const_local_tl(a->u4);
    GEN_CODE_ADD_t(tcg_Ri, ti, 0);
    tcg_temp_free(ti);
}))
#endif

FR_GEN_CODE(ADD2, "ADD #s5, Ri", "1", FLAG_UPDATE_NZVC,
({
    TCGv ti = tcg_const_local_tl(a->s5);
    GEN_CODE_ADD_t(tcg_Ri, ti, 0);
    tcg_temp_free(ti);
    if (a->s5)
        WRITTING_R15(a->ri);
}))

FR_GEN_CODE(ADDC, "ADDC Rj, Ri", "1", FLAG_UPDATE_NZVC,
({
    GEN_CODE_ADD_t(tcg_Ri, tcg_Rj, 1);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(ADDN, "ADDN Rj, Ri", "1", FLAG_NONE,
({
    tcg_gen_add_tl(tcg_Ri, tcg_Ri, tcg_Rj);
    WRITTING_R15(a->ri);
}))

#if 0  //merged to ADDN2
FR_GEN_CODE(ADDN1, "ADDN #i4, Ri", "1", FLAG_NONE,
({
    tcg_gen_addi_tl(tcg_Ri, tcg_Ri, a->u4);
}))
#endif

FR_GEN_CODE(ADDN2, "ADDN #s5, Ri", "1", FLAG_NONE,
({
    tcg_gen_addi_tl(tcg_Ri, tcg_Ri, a->s5);
    if (a->s5)
        WRITTING_R15(a->ri);
}))

static inline void GEN_CODE_SUB_t(TCGv r1, TCGv r2, bool carry, bool is_cmp)
{
    TCGv t0 = tcg_const_local_tl(0);
    TCGv tmp = tcg_temp_local_new();
    if (carry) {
        tcg_gen_sub2_tl(tmp, cpu_PS_C, (r1), t0, cpu_PS_C, t0);
        tcg_gen_sub2_tl(tmp, cpu_PS_C, (tmp), cpu_PS_C, (r2), t0);
    } else {
        tcg_gen_sub2_tl(tmp, cpu_PS_C, (r1), t0, (r2), t0);
    }
    tcg_gen_setcondi_tl(TCG_COND_NE, cpu_PS_C, cpu_PS_C, 0);
    tcg_gen_flag_nz(tmp, BITS_WORD);
    tcg_gen_sub_flag_v(tmp, (r1), (r2));
    if (!is_cmp) {
        tcg_gen_mov_tl((r1), tmp);
    }

    tcg_temp_free(t0);
    tcg_temp_free(tmp);
}

FR_GEN_CODE(SUB, "SUB Rj, Ri", "1", FLAG_UPDATE_NZVC,
({
    GEN_CODE_SUB_t(tcg_Ri, tcg_Rj, 0, 0);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(SUBC, "SUBC Rj, Ri", "1", FLAG_UPDATE_NZVC,
({
    GEN_CODE_SUB_t(tcg_Ri, tcg_Rj, 1, 0);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(SUBN, "SUBN Rj, Ri", "1", FLAG_NONE,
({
    tcg_gen_sub_tl(tcg_Ri, tcg_Ri, tcg_Rj);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(CMP, "CMP Rj, Ri", "1", FLAG_UPDATE_NZVC,
({
    GEN_CODE_SUB_t(tcg_Ri, tcg_Rj, 0, 1);
}))

#if 0  //merged to CMP2
FR_GEN_CODE(CMP1, "CMP #i4, Ri", "1", FLAG_UPDATE_NZVC,
({
    TCGv ti = tcg_const_local_tl(a->u4);
    GEN_CODE_SUB_t(tcg_Ri, ti, 0, 1);
    tcg_temp_free(ti);
}))
#endif

FR_GEN_CODE(CMP2, "CMP #s5, Ri", "1", FLAG_UPDATE_NZVC,
({
    TCGv ti = tcg_const_local_tl(a->s5);
    GEN_CODE_SUB_t(tcg_Ri, ti, 0, 1);
    tcg_temp_free(ti);
}))

FR_GEN_CODE(AND, "AND Rj, Ri", "1", FLAG_UPDATE_NZ,
({
    tcg_gen_and_tl(tcg_Ri, tcg_Ri, tcg_Rj);
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(AND1, "AND Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld32u(tmp, tcg_Ri, 0);
    tcg_gen_and_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st32(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_WORD);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ANDH, "ANDH Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld16u(tmp, tcg_Ri, 0);
    tcg_gen_and_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st16(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_HALFWORD);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ANDB, "ANDB Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_and_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_BYTE);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(OR, "OR Rj, Ri", "1", FLAG_UPDATE_NZ,
({
    tcg_gen_or_tl(tcg_Ri, tcg_Ri, tcg_Rj);
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
    WRITTING_R15(a->ri);
}))
FR_GEN_CODE(OR1, "OR Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld32u(tmp, tcg_Ri, 0);
    tcg_gen_or_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st32(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_WORD);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ORH, "ORH Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld16u(tmp, tcg_Ri, 0);
    tcg_gen_or_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st16(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_HALFWORD);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ORB, "ORB Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_or_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_BYTE);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(EOR, "EOR Rj, Ri", "1", FLAG_UPDATE_NZ,
({
    tcg_gen_xor_tl(tcg_Ri, tcg_Ri, tcg_Rj);
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(EOR1, "EOR Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld32u(tmp, tcg_Ri, 0);
    tcg_gen_xor_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st32(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_WORD);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(EORH, "EORH Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld16u(tmp, tcg_Ri, 0);
    tcg_gen_xor_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st16(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_HALFWORD);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(EORB, "EORB Rj, @Ri", "1+2a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_xor_tl(tmp, tmp, tcg_Rj);
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_gen_flag_nz(tmp, BITS_BYTE);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BANDL, "BANDL #u4, @Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_andi_tl(tmp, tmp, ((a->u4)|0xF0));
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BANDH, "BANDH #u4, @Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_andi_tl(tmp, tmp, ((a->u4<<4)|0xF));
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BORL, "BORL #u4, @Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_ori_tl(tmp, tmp, ((a->u4)|0xF0));
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BORH, "BORH #u4, @Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_ori_tl(tmp, tmp, ((a->u4<<4)|0xF));
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BEORL, "BEORL #u4, @Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_xori_tl(tmp, tmp, ((a->u4<<4)|0xF));
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BEORH, "BEORH #u4, @Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_xori_tl(tmp, tmp, ((a->u4<<4)|0xF));
    tcg_gen_qemu_st8(tmp, tcg_Ri, 0);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BTSTL, "BTSTL #u4, @Ri", "2+a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_andi_tl(tmp, tmp, a->u4);
    tcg_gen_flag_nz_2(tmp);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(BTSTH, "BTSTH #u4, @Ri", "2+a", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_qemu_ld8u(tmp, tcg_Ri, 0);
    tcg_gen_andi_tl(tmp, tmp, a->u4<<4);
    tcg_gen_flag_nz(tmp, BITS_BYTE);
    tcg_temp_free(tmp);
}))

FR_GEN_CODE(MUL, "MUL Rj, Ri", "5", (FLAG_UPDATE_NZV|FLAG_NOT_IN_DELAY_SLOT),
({
    tcg_gen_muls2_tl(tcg_R(R_MDL), tcg_R(R_MDH), tcg_Ri, tcg_Rj);

    tcg_gen_shri_tl(cpu_PS_N, tcg_R(R_MDL), MSB_BIT_WORD);
    tcg_gen_or_tl(cpu_PS_Z, tcg_R(R_MDL), tcg_R(R_MDH));
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, cpu_PS_Z, 0);

    tcg_gen_add_tl(cpu_PS_V, cpu_PS_N, tcg_R(R_MDH)); // 11..11 1xxx..xxxx <=> 00..00 0xxx..xxxx
    tcg_gen_setcondi_tl(TCG_COND_NE, cpu_PS_V, cpu_PS_V, 0);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(MULU, "MULU Rj, Ri", "5", (FLAG_UPDATE_NZV|FLAG_NOT_IN_DELAY_SLOT),
({
    tcg_gen_mulu2_tl(tcg_R(R_MDL), tcg_R(R_MDH), tcg_Ri, tcg_Rj);

    tcg_gen_shri_tl(cpu_PS_N, tcg_R(R_MDL), MSB_BIT_WORD);
    tcg_gen_or_tl(cpu_PS_Z, tcg_R(R_MDL), tcg_R(R_MDH));
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, cpu_PS_Z, 0);
    tcg_gen_setcondi_tl(TCG_COND_NE, cpu_PS_V, tcg_R(R_MDH), 0);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(MULH, "MULH Rj, Ri", "3", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp1 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    tcg_gen_ext16s_tl(tmp1, tcg_Ri);
    tcg_gen_ext16s_tl(tmp2, tcg_Rj);
    tcg_gen_mul_tl(tcg_R(R_MDL), tmp1, tmp2);

    tcg_gen_flag_nz(tcg_R(R_MDL), BITS_WORD);

    tcg_temp_free(tmp1);
    tcg_temp_free(tmp2);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(MULUH, "MULUH Rj, Ri", "3", (FLAG_UPDATE_NZ|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp1 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    tcg_gen_ext16u_tl(tmp1, tcg_Ri);
    tcg_gen_ext16u_tl(tmp2, tcg_Rj);
    tcg_gen_mul_tl(tcg_R(R_MDL), tmp1, tmp2);

    tcg_gen_flag_nz(tcg_R(R_MDL), BITS_WORD);

    tcg_temp_free(tmp1);
    tcg_temp_free(tmp2);
    WRITTING_R15(a->ri);
}))

// TODO: use tcg_gen_div_tl/tcg_gen_divu_tl
FR_GEN_CODE(DIV0S, "DIV0S Ri", "1", FLAG_D,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_shri_tl(tmp, tcg_R(R_MDL), MSB_BIT_WORD);
    tcg_gen_deposit_tl(tcg_PS, tcg_PS, tmp, R_PS_SCR_D0_BIT, 1); // MDL[31] => D0

    tcg_gen_xor_tl(tmp, tcg_R(R_MDL), tcg_Ri);
    tcg_gen_shri_tl(tmp, tmp, MSB_BIT_WORD);
    tcg_gen_deposit_tl(tcg_PS, tcg_PS, tmp, R_PS_SCR_D1_BIT, 1); // MDL[31] eor Ri[31] => D1

    tcg_gen_sari_tl(tcg_R(R_MDH), tcg_R(R_MDL), MSB_BIT_WORD); // exts(MDL) => MDH, MDL
    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(DIV0U, "DIV0U Ri", "1", FLAG_D,
({
    TCGv t0 = tcg_const_local_tl(0);
    tcg_gen_deposit_tl(tcg_PS, tcg_PS, t0, R_PS_SCR_D0_BIT, 2);
    tcg_gen_movi_tl(tcg_R(R_MDH), 0);
    tcg_temp_free(t0);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(DIV1, "DIV1 Ri", "d", FLAG_UPDATE_ZC,
({
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv t0 = tcg_const_local_tl(0);
    TCGv d0 = tcg_temp_local_new();
    TCGv d1 = tcg_temp_local_new();
    TCGLabel *l1 = gen_new_label();
    TCGLabel *l2 = gen_new_label();
    TCGLabel *l3 = gen_new_label();
    tcg_gen_shri_tl(tmp, tcg_R(R_MDL), MSB_BIT_WORD);
    tcg_gen_shli_tl(tcg_R(R_MDL), tcg_R(R_MDL), 1);
    tcg_gen_shli_tl(tcg_R(R_MDH), tcg_R(R_MDH), 1);
    tcg_gen_and_tl(tcg_R(R_MDH), tcg_R(R_MDH), tmp);

    tcg_gen_extract_tl(d0, tcg_R(R_PS), R_PS_SCR_D0_BIT, 1);
    tcg_gen_extract_tl(d1, tcg_R(R_PS), R_PS_SCR_D1_BIT, 1);

    tcg_gen_brcondi_tl(TCG_COND_NE, d1, 1, l1); // if D1 != 1: goto l1
    tcg_gen_add2_tl(tmp, cpu_PS_C, tcg_R(R_MDH), t0, tcg_Ri, t0);
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, tmp, 0);
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, d1, 1, l2);
    gen_set_label(l1);
    tcg_gen_sub2_tl(tmp, cpu_PS_C, tcg_R(R_MDH), t0, tcg_Ri, t0);
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, tmp, 0);
    tcg_gen_setcondi_tl(TCG_COND_NE, cpu_PS_C, cpu_PS_C, 0);
    gen_set_label(l2);

    tcg_gen_xor_tl(tmp1, d0, d1);
    tcg_gen_xor_tl(tmp1, tmp1, cpu_PS_C);
    tcg_gen_brcondi_tl(TCG_COND_NE, tmp1, 0, l3);

    tcg_gen_mov_tl(tcg_R(R_MDH), tmp);
    tcg_gen_movi_tl(tcg_R(R_MDL), 1);
    gen_set_label(l3);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp1);
    tcg_temp_free(t0);
    tcg_temp_free(d0);
    tcg_temp_free(d1);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(DIV2, "DIV2 Ri", "1", FLAG_UPDATE_ZC,
({
    TCGv tmp = tcg_temp_local_new();
    TCGv t0 = tcg_const_local_tl(0);
    TCGv d1 = tcg_temp_local_new();
    TCGLabel *l1 = gen_new_label();
    TCGLabel *l2 = gen_new_label();
    TCGLabel *l3 = gen_new_label();
    tcg_gen_extract_tl(d1, tcg_R(R_PS), R_PS_SCR_D1_BIT, 1);

    tcg_gen_brcondi_tl(TCG_COND_NE, d1, 1, l1); // if D1 != 1: goto l1
    tcg_gen_add2_tl(tmp, cpu_PS_C, tcg_R(R_MDH), t0, tcg_Ri, t0);
    //tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, tmp, 0);
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, d1, 1, l2);
    gen_set_label(l1);
    tcg_gen_sub2_tl(tmp, cpu_PS_C, tcg_R(R_MDH), t0, tcg_Ri, t0);
    //tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_PS_Z, tmp, 0);
    tcg_gen_setcondi_tl(TCG_COND_NE, cpu_PS_C, cpu_PS_C, 0);
    gen_set_label(l2);

    tcg_gen_movi_tl(cpu_PS_Z, 1);
    //tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_PS_Z, 0, l3); // if Z == 0: goto l3
    tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l3); // if tmp (which is ~Z) == 0: goto l3
    tcg_gen_movi_tl(tcg_R(R_MDH), 0);
    tcg_gen_movi_tl(cpu_PS_Z, 0);
    gen_set_label(l3);

    tcg_temp_free(tmp);
    tcg_temp_free(t0);
    tcg_temp_free(d1);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(DIV3, "DIV3", "1", FLAG_NONE,
({
    //tcg_gen_setcondi_tl(TCG_COND_NE, cpu_PS_Z, cpu_PS_Z, 0);
    tcg_gen_add_tl(tcg_R(R_MDL), tcg_R(R_MDL), cpu_PS_Z);
}))

FR_GEN_CODE(DIV4S, "DIV4S", "1", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();

    TCGLabel *l1 = gen_new_label();
    tcg_gen_extract_tl(tmp, tcg_PS, R_PS_SCR_D1_BIT, 1);
    tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 1, l1); // if (D1 == 1): 0-MDL => MDL
    tcg_gen_neg_tl(tcg_R(R_MDL), tcg_R(R_MDL));
    gen_set_label(l1);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(LSL, "LSL Rj, Ri", "1", FLAG_UPDATE_NZC,
({
    TCGLabel *l1 = gen_new_label();
    TCGv tmp = tcg_temp_local_new();

    tcg_gen_movi_tl(cpu_PS_C, 0);
    tcg_gen_andi_tl(tmp, tcg_Rj, 0x1f);
    tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l1); // if Rj == 0: goto l1
    tcg_gen_subi_tl(tmp, tmp, 1);
    tcg_gen_shl_tl(tcg_Ri, tcg_Ri, tmp);
    tcg_gen_shri_tl(cpu_PS_C, tcg_Ri, MSB_BIT_WORD);
    tcg_gen_shli_tl(tcg_Ri, tcg_Ri, 1);

    gen_set_label(l1);
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LSL2, "LSL #u5, Ri", "1", FLAG_UPDATE_NZC,
({
    if (a->u5 != 0) {
        tcg_gen_extract_tl(cpu_PS_C, tcg_Ri, (BITS_WORD - a->u5), 1);
        tcg_gen_shli_tl(tcg_Ri, tcg_Ri, a->u5);
        WRITTING_R15(a->ri);
    }
    else {
        tcg_gen_movi_tl(cpu_PS_C, 0);
    }
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
}))

#if 0 //merged to LSL
FR_GEN_CODE(LSL2, "LSL2 #u4, Ri", "1", FLAG_UPDATE_NZC,
({
    tcg_gen_extract_tl(cpu_PS_C, tcg_Ri, (BITS_WORD - a->u4), 1);
    tcg_gen_shli_tl(tcg_Ri, tcg_Ri, a->u4);
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
}))
#endif

FR_GEN_CODE(LSR, "LSR Rj, Ri", "1", FLAG_UPDATE_NZC,
({
    TCGLabel *l1 = gen_new_label();
    TCGv tmp = tcg_temp_local_new();

    tcg_gen_movi_tl(cpu_PS_C, 0);
    tcg_gen_andi_tl(tmp, tcg_Rj, 0x1f);
    tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l1); // if Rj == 0: goto l1

    tcg_gen_subi_tl(tmp, tmp, 1);
    tcg_gen_shr_tl(tcg_Ri, tcg_Ri, tmp);
    tcg_gen_andi_tl(cpu_PS_C, tcg_Ri, 1);
    tcg_gen_shri_tl(tcg_Ri, tcg_Ri, 1);

    gen_set_label(l1);
    tcg_gen_flag_nz_2(tcg_Ri);
    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LSR2, "LSR #u5, Ri", "1", FLAG_UPDATE_NZC,
({
    if (a->u5 != 0) {
        tcg_gen_extract_tl(cpu_PS_C, tcg_Ri, (a->u5-1), 1);
        tcg_gen_shri_tl(tcg_Ri, tcg_Ri, a->u5);
        WRITTING_R15(a->ri);
    }
    else {
        tcg_gen_movi_tl(cpu_PS_C, 0);
    }
    tcg_gen_flag_nz_2(tcg_Ri);
}))

#if 0  //merged to LSR2
FR_GEN_CODE(LSR2, "LSR2 #u4, Ri", "1", FLAG_UPDATE_NZC,
({
    tcg_gen_extract_tl(cpu_PS_C, tcg_Ri, (a->u4-1), 1);
    tcg_gen_shri_tl(tcg_Ri, tcg_Ri, a->u4);

    tcg_gen_flag_nz_2(tcg_Ri);
}))
#endif

FR_GEN_CODE(ASR, "ASR Rj, Ri", "1", FLAG_UPDATE_NZC,
({
    TCGLabel *l1 = gen_new_label();
    TCGv tmp = tcg_temp_local_new();

    tcg_gen_movi_tl(cpu_PS_C, 0);
    tcg_gen_andi_tl(tmp, tcg_Rj, 0x1f);
    tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l1); // if Rj == 0: goto l1

    tcg_gen_subi_tl(tmp, tmp, 1);
    tcg_gen_sar_tl(tcg_Ri, tcg_Ri, tmp);
    tcg_gen_andi_tl(cpu_PS_C, tcg_Ri, 1);
    tcg_gen_sari_tl(tcg_Ri, tcg_Ri, 1);

    gen_set_label(l1);
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(ASR2, "ASR #u5, Ri", "1", FLAG_UPDATE_NZC,
({
    if (a->u5 != 0) {
        tcg_gen_extract_tl(cpu_PS_C, tcg_Ri, (a->u5-1), 1);
        tcg_gen_sari_tl(tcg_Ri, tcg_Ri, a->u5);
        WRITTING_R15(a->ri);
    }
    else {
        tcg_gen_movi_tl(cpu_PS_C, 0);
    }
    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
}))

#if 0  //merged to ASR2
FR_GEN_CODE(ASR2, "ASR2 #u4, Ri", "1", FLAG_UPDATE_NZC,
({
    tcg_gen_extract_tl(cpu_PS_C, tcg_Ri, (a->u4-1), 1);
    tcg_gen_sari_tl(tcg_Ri, tcg_Ri, a->u4);

    tcg_gen_flag_nz(tcg_Ri, BITS_WORD);
}))
#endif

FR_GEN_CODE(LDI32, "LDI:32 #i32, Ri", "3", FLAG_NOT_IN_DELAY_SLOT,
({
    tcg_gen_movi_tl(tcg_Ri, a->i32);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDI20, "LDI:20 #i20, Ri", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    tcg_gen_movi_tl(tcg_Ri, a->i20);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDI8, "LDI:8 #i8, Ri", "1", FLAG_NONE,
({
    tcg_gen_movi_tl(tcg_Ri, a->i8);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LD, "LD @Rj, Ri", "b", FLAG_NONE,
({
    tcg_gen_qemu_ld32u(tcg_Ri, tcg_Rj, 0);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LD1, "LD @(R13, Rj), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_tl(tmp, tcg_R(R_13), tcg_Rj);
    tcg_gen_qemu_ld32u(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LD2, "LD @(R14, disp10), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->disp10);
    tcg_gen_qemu_ld32u(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LD3, "LD @(R15, udisp6), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_15), a->udisp6);
    tcg_gen_qemu_ld32u(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LD4, "LD @R15+, Ri", "b", FLAG_NONE,
({
    tcg_gen_qemu_ld32u(tcg_Ri, tcg_R(R_15), 0);
    if (a->ri != R_15) {
        tcg_gen_addi_tl(tcg_R(R_15), tcg_R(R_15), 4);
    }
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(LD5, "LD @R15+, Rs", "b", FLAG_NONE,
({
    if (a->rs+DR_BASE <= DR_END) {
        tcg_gen_qemu_ld32u(tcg_Rs, tcg_R(R_15), 0);

        TCGv tmp = tcg_temp_local_new();
        TCGLabel *l1 = gen_new_label();
        TCGLabel *l2 = gen_new_label();
        if (a->rs == R_USP ||  a->rs == R_SSP) {
            tcg_gen_andi_tl(tmp, tcg_PS, R_PS_CCR_S); // S: 0 is SSP, 1 is USP.
            if (a->rs == R_USP) {
                tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l1); // if a->rs is R_USP and flag_S is USP: goto l1
            } else if (a->rs == R_SSP) {
                tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l1); // if a->rs is R_SSP and flag_S is SSP: goto l1
            }
            tcg_gen_addi_tl(tcg_R(R_15), tcg_R(R_15), 4);

            // special handler. cannot call WRITTING_R15/_SP to set flag
            tcg_gen_mov_tl((a->rs==R_USP)?tcg_R(R_SSP):tcg_R(R_USP), tcg_R(R_15)); // sync R15 to USP/SSP accordingly
            tcg_gen_brcondi_tl(TCG_COND_ALWAYS, tmp, 0, l2);
            gen_set_label(l1);
            tcg_gen_mov_tl(tcg_R(R_15), tcg_Rs);
            gen_set_label(l2);
        }
        else {
            tcg_gen_addi_tl(tcg_R(R_15), tcg_R(R_15), 4);
            WRITTING_R15(R_15);
        }
        tcg_temp_free(tmp);
    }
}))

FR_GEN_CODE(LD6, "LD @R15+, PS", "1+a+c", (FLAG_NOT_IN_DELAY_SLOT|FLAG_CCR_INVALID),
({
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGLabel *l1 = gen_new_label();
    TCGLabel *l2 = gen_new_label();
    tcg_gen_andi_tl(tmp1, tcg_PS, R_PS_CCR_S); // S: 0 is SSP, 1 is USP. keep the old value
    tcg_gen_andi_tl(tcg_PS, tcg_PS, R_PS_ILM4); // Keep the bit ILM4
    tcg_gen_qemu_ld32u(tmp, tcg_R(R_15), 0);
    tcg_gen_or_tl(tcg_PS, tcg_PS, tmp);

    tcg_gen_brcondi_tl(TCG_COND_NE, tmp1, 0, l1); // old flag_S is USP: goto l1
    tcg_gen_addi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, tmp1, 0, l2);
    gen_set_label(l1);
    tcg_gen_addi_tl(tcg_R(R_USP), tcg_R(R_USP), 4);
    gen_set_label(l2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp1);
    WRITTING_SP(R_SSP);
}))

FR_GEN_CODE(LDUH, "LDUH @Rj, Ri", "b", FLAG_NONE,
({
    tcg_gen_qemu_ld16u(tcg_Ri, tcg_Rj, 0);
    tcg_gen_ext16u_tl(tcg_Ri, tcg_Ri);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDUH1, "LDUH @(R13, Rj), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_tl(tmp, tcg_R(R_13), tcg_Rj);
    tcg_gen_qemu_ld16u(tcg_Ri, tmp, 0);
    tcg_gen_ext16u_tl(tcg_Ri, tcg_Ri);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDUH2, "LDUH @(R14, disp9), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->disp9);
    tcg_gen_qemu_ld16u(tcg_Ri, tmp, 0);
    tcg_gen_ext16u_tl(tcg_Ri, tcg_Ri);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDUB, "LDUB @Rj, Ri", "b", FLAG_NONE,
({
    tcg_gen_qemu_ld8u(tcg_Ri, tcg_Rj, 0);
    tcg_gen_ext8u_tl(tcg_Ri, tcg_Ri);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDUB1, "LDUB @(R13, Rj), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_tl(tmp, tcg_R(R_13), tcg_Rj);
    tcg_gen_qemu_ld8u(tcg_Ri, tmp, 0);
    tcg_gen_ext8u_tl(tcg_Ri, tcg_Ri);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDUB2, "LDUB @(R14, disp8), Ri", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->disp8);
    tcg_gen_qemu_ld8u(tcg_Ri, tmp, 0);
    tcg_gen_ext8u_tl(tcg_Ri, tcg_Ri);

    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(ST, "ST Ri, @Rj", "a", FLAG_NONE,
({
    tcg_gen_qemu_st32(tcg_Ri, tcg_Rj, 0);
}))

FR_GEN_CODE(ST1, "ST Ri, @(R13, Rj)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_tl(tmp, tcg_R(R_13), tcg_Rj);
    tcg_gen_qemu_st32(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ST2, "ST Ri, @(R14, disp10)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->disp10);
    tcg_gen_qemu_st32(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ST3, "ST Ri, @(R15, udisp6)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->udisp6);
    tcg_gen_qemu_st32(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(ST4, "ST Ri, @-R15", "a", FLAG_NONE,
({
    if (a->ri == R_15) {
        TCGv tmp = tcg_temp_local_new();
        tcg_gen_mov_tl(tmp, tcg_R(R_15));
        tcg_gen_subi_tl(tcg_R(R_15), tcg_R(R_15), 4);
        tcg_gen_qemu_st32(tmp, tcg_R(R_15), 0);

        tcg_temp_free(tmp);
    }
    else {
        tcg_gen_subi_tl(tcg_R(R_15), tcg_R(R_15), 4);
        tcg_gen_qemu_st32(tcg_Ri, tcg_R(R_15), 0);
    }
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(ST5, "ST Rs, @-R15", "a", FLAG_NONE,
({
    if (a->rs+DR_BASE <= DR_END) {
        tcg_gen_subi_tl(tcg_R(R_15), tcg_R(R_15), 4);
        tcg_gen_qemu_st32(tcg_Rs, tcg_R(R_15), 0);
        WRITTING_R15(R_15);
    }
}))

FR_GEN_CODE(ST6, "ST PS, @-R15", "a", FLAG_CCR_SYNC,
({
    tcg_gen_subi_tl(tcg_R(R_15), tcg_R(R_15), 4);
    tcg_gen_qemu_st32(tcg_PS, tcg_R(R_15), 0);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(STH, "STH Ri, @Rj", "a", FLAG_NONE,
({
    tcg_gen_qemu_st16(tcg_Ri, tcg_Rj, 0);
}))

FR_GEN_CODE(STH1, "STH Ri, @(R13, Rj)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_tl(tmp, tcg_R(R_13), tcg_Rj);
    tcg_gen_qemu_st16(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(STH2, "STH Ri, @(R14, disp9)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->disp9);
    tcg_gen_qemu_st16(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(STB, "STB Ri, @Rj", "a", FLAG_NONE,
({
    tcg_gen_qemu_st8(tcg_Ri, tcg_Rj, 0);
}))

FR_GEN_CODE(STB1, "STB Ri, @(R13, Rj)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_tl(tmp, tcg_R(R_13), tcg_Rj);
    tcg_gen_qemu_st8(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(STB2, "STB Ri, @(R14, disp8)", "a", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tmp, tcg_R(R_14), a->disp8);
    tcg_gen_qemu_st8(tcg_Ri, tmp, 0);

    tcg_temp_free(tmp);
}))

FR_GEN_CODE(MOV, "MOV Rj, Ri", "1", FLAG_NONE,
({
    tcg_gen_mov_tl(tcg_Ri, tcg_Rj);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(MOV1, "MOV Rs, Ri", "1", FLAG_NONE,
({
    if (a->rs+DR_BASE <= DR_END) {
        tcg_gen_mov_tl(tcg_Ri, tcg_Rs);
        WRITTING_R15(a->ri);
    }
}))

FR_GEN_CODE(MOV2, "MOV PS, Ri", "1", FLAG_CCR_SYNC,
({
    tcg_gen_mov_tl(tcg_Ri, tcg_PS);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(MOV3, "MOV Ri, Rs", "1", FLAG_NONE,
({
    if (a->rs+DR_BASE <= DR_END) {
        tcg_gen_mov_tl(tcg_Rs, tcg_Ri);
        WRITTING_SP(a->rs+DR_BASE);
    }
}))

FR_GEN_CODE(MOV4, "MOV Ri, PS", "c", FLAG_CCR_INVALID,
({
    tcg_gen_andi_tl(tcg_PS, tcg_PS, R_PS_ILM4); // Keep the bit ILM4
    tcg_gen_or_tl(tcg_PS, tcg_PS, tcg_Ri);
    WRITTING_SP(R_SSP);
}))

FR_GEN_CODE(JMP, "JMP @Ri", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    dc->base.is_jmp = DISAS_TB_JUMP;
    tcg_gen_mov_tl(tcg_PC, tcg_Ri);
}))

FR_GEN_CODE(CALL, "CALL label12", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    tcg_gen_movi_tl(tcg_R(R_RP), dc->pc+2);
    tcg_gen_movi_tl(tcg_PC, dc->pc+2+a->label12);
    dc->base.is_jmp = DISAS_TB_JUMP;
}))

FR_GEN_CODE(CALL1, "CALL @Ri", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    tcg_gen_movi_tl(tcg_R(R_RP), dc->pc+2);
    tcg_gen_mov_tl(tcg_PC, tcg_Ri);
    dc->base.is_jmp = DISAS_TB_JUMP;
}))

FR_GEN_CODE(RET, "RET", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    tcg_gen_mov_tl(tcg_PC, tcg_R(R_RP));
    dc->base.is_jmp = DISAS_TB_JUMP;
}))

FR_GEN_CODE(INT, "INT #u8", "3+3a", (FLAG_SI|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_subi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);
    tcg_gen_qemu_st32(tcg_PS, tcg_R(R_SSP), 0);
    tcg_gen_andi_tl(tcg_PS, tcg_PS, ~(R_PS_CCR_I|R_PS_CCR_S)); // clear bit

    tcg_gen_subi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);
    tcg_gen_movi_tl(tcg_PC, dc->pc+2);
    tcg_gen_qemu_st32(tcg_PC, tcg_R(R_SSP), 0);

    tcg_gen_addi_tl(tmp, tcg_R(R_TBR), 0x3FC-4*a->u8);
    tcg_gen_qemu_ld32u(tcg_PC, tmp, 0);

    tcg_gen_mov_tl(tcg_R(R_15), tcg_R(R_SSP));
    dc->base.is_jmp = DISAS_TB_JUMP;
}))

FR_GEN_CODE(INTE, "INTE", "3+3a", (FLAG_SI|FLAG_NOT_IN_DELAY_SLOT),
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_subi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);
    tcg_gen_qemu_st32(tcg_PS, tcg_R(R_SSP), 0);
    tcg_gen_andi_tl(tcg_PS, tcg_PS, ~(R_PS_CCR_S|R_PS_ILM)); // clear bit
    tcg_gen_ori_tl(tcg_PS, tcg_PS, 4<<R_PS_ILM0_BIT);

    tcg_gen_subi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);
    tcg_gen_movi_tl(tcg_PC, dc->pc+2);
    tcg_gen_qemu_st32(tcg_PC, tcg_R(R_SSP), 0);

    tcg_gen_addi_tl(tmp, tcg_R(R_TBR), 0x3D8);
    tcg_gen_qemu_ld32u(tcg_PC, tmp, 0);

    tcg_gen_mov_tl(tcg_R(R_15), tcg_R(R_SSP));
    dc->base.is_jmp = DISAS_TB_JUMP;
}))

FR_GEN_CODE(RETI, "RETI", "2+2a", (FLAG_NOT_IN_DELAY_SLOT|FLAG_CCR_INVALID),
({
    TCGv tmp = tcg_temp_local_new();
    TCGLabel *l1 = gen_new_label();
    tcg_gen_andi_tl(tmp, tcg_PS, R_PS_CCR_S); // S: 0 is SSP, 1 is USP.
    tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l1); // if is SSP: goto l1
    gen_helper_raise_illegal_instruction(cpu_env);
    gen_set_label(l1);

    tcg_gen_qemu_ld32u(tcg_PC, tcg_R(R_SSP), 0);
    tcg_gen_addi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);

    tcg_gen_andi_tl(tcg_PS, tcg_PS, R_PS_ILM4); // Keep the bit ILM4
    tcg_gen_qemu_ld32u(tmp, tcg_R(R_SSP), 0);
    tcg_gen_or_tl(tcg_PS, tcg_PS, tmp);
    tcg_gen_addi_tl(tcg_R(R_SSP), tcg_R(R_SSP), 4);

    tcg_temp_free(tmp);
    WRITTING_SP(R_SSP);
    dc->base.is_jmp = DISAS_TB_JUMP;
}))

FR_GEN_CODE(Bcc, "Bcc label9", "2/1", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGLabel *label1 = gen_new_label();
    TCGLabel *label2 = gen_new_label();
    DisasCompare cmp;
    fr_test_cc(&cmp, a->cc);
    fr_jump_cc(&cmp, label1);
    fr_free_cc(&cmp);
    tcg_gen_movi_tl(tcg_PC, dc->pc+2);
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, tcg_PC, 0, label2);

    gen_set_label(label1);
    tcg_gen_movi_tl(tcg_PC, dc->pc+2+a->label9);
    gen_set_label(label2);
    dc->base.is_jmp = DISAS_TB_JUMP;

}))

FR_GEN_CODE(JMPD, "JMP:D @Ri", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    dc->dpc = true;
    tcg_gen_mov_tl(dpc, tcg_Ri);
}))

FR_GEN_CODE(CALLD, "CALL:D label12", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    dc->dpc = true;
    tcg_gen_movi_tl(tcg_R(R_RP), dc->pc+4);
    tcg_gen_movi_tl(dpc, dc->pc+2+a->label12);
}))

FR_GEN_CODE(CALLD1, "CALL:D @Ri", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    dc->dpc = true;
    tcg_gen_movi_tl(tcg_R(R_RP), dc->pc+4);
    tcg_gen_mov_tl(dpc, tcg_Ri);
}))

FR_GEN_CODE(RETD, "RET:D", "2", FLAG_NOT_IN_DELAY_SLOT,
({
    dc->dpc = true;
    tcg_gen_mov_tl(dpc, tcg_R(R_RP));
}))

FR_GEN_CODE(BccD, "Bcc:D label9", "2/1", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGLabel *label = gen_new_label();
    TCGLabel *label2 = gen_new_label();
    DisasCompare cmp;
    fr_test_cc(&cmp, a->cc);
    fr_jump_cc(&cmp, label);
    fr_free_cc(&cmp);
    tcg_gen_movi_tl(dpc, dc->pc+4);
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, tcg_PC, 0, label2);

    gen_set_label(label);
    tcg_gen_movi_tl(dpc, dc->pc+2+a->label9);
    gen_set_label(label2);
    dc->dpc = true;
}))

FR_GEN_CODE(DMOV, "DMOV @dir10, R13", "b", FLAG_NONE,
({
    TCGv t0 = tcg_const_local_tl(a->dir10);
    tcg_gen_qemu_ld32u(tcg_R(R_13), t0, 0);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOV1, "DMOV R13, @dir10", "a", FLAG_NONE,
({
    TCGv t0 = tcg_const_local_tl(a->dir10);
    tcg_gen_qemu_st32(tcg_R(R_13), t0, 0);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOV2, "DMOV @dir10, @R13+", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir10);
    tcg_gen_qemu_ld32u(tcg_R(R_13), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_13), tcg_R(R_13), 4);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOV3, "DMOV @R13+, @dir10", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir10);
    tcg_gen_qemu_st32(tcg_R(R_13), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_13), tcg_R(R_13), 4);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOV4, "DMOV @dir10, @-R15", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir10);
    tcg_gen_subi_tl(tcg_R(R_15), tcg_R(R_15), 4);
    tcg_gen_qemu_ld32u(tcg_R(R_15), t0, 0);
    tcg_temp_free(t0);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(DMOV5, "DMOV @R15+, @dir10", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir10);
    tcg_gen_qemu_st32(tcg_R(R_15), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_15), tcg_R(R_15), 4);
    tcg_temp_free(t0);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(DMOVH, "DMOVH @dir9, R13", "b", FLAG_NONE,
({
    TCGv t0 = tcg_const_local_tl(a->dir9);
    tcg_gen_qemu_ld16u(tcg_R(R_13), t0, 0);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVH1, "DMOVH R13, @dir9", "a", FLAG_NONE,
({
    TCGv t0 = tcg_const_local_tl(a->dir9);
    tcg_gen_qemu_st16(tcg_R(R_13), t0, 0);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVH2, "DMOVH @dir9, @R13+", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir9);
    tcg_gen_qemu_ld16u(tcg_R(R_13), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_13), tcg_R(R_13), 2);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVH3, "DMOVH @R13+, @dir9", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir9);
    tcg_gen_qemu_st16(tcg_R(R_13), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_13), tcg_R(R_13), 2);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVB, "DMOVB @dir8, R13", "b", FLAG_NONE,
({
    TCGv t0 = tcg_const_local_tl(a->dir8);
    tcg_gen_qemu_ld8u(tcg_R(R_13), t0, 0);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVB1, "DMOVB R13, @dir8", "a", FLAG_NONE,
({
    TCGv t0 = tcg_const_local_tl(a->dir8);
    tcg_gen_qemu_st8(tcg_R(R_13), t0, 0);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVB2, "DMOVB @dir8, @R13+", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv t0 = tcg_const_local_tl(a->dir8);
    tcg_gen_qemu_ld8u(tcg_R(R_13), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_13), tcg_R(R_13), 1);
    tcg_temp_free(t0);
}))

FR_GEN_CODE(DMOVB3, "DMOVB @R13+, @dir8", "2a", FLAG_NOT_IN_DELAY_SLOT ,
({
    TCGv t0 = tcg_const_local_tl(a->dir8);
    tcg_gen_qemu_st8(tcg_R(R_13), t0, 0);
    tcg_gen_addi_tl(tcg_R(R_13), tcg_R(R_13), 1);
    tcg_temp_free(t0);
}))

// (LDRES, "LDRES @Ri+, #u4", "a", FLAG_NONE, {}))
// (STRES, "STRES #u4, @Ri+", "a", FLAG_NONE, {}))
FR_GEN_CODE(COP, "LDRES/STRES", "a", FLAG_NONE,
({
    qemu_log_mask(LOG_UNIMP, "%s: LDRES/STRES is not supported\n", __func__);
}))

// (COPOP, "COPOP #u4, #CC, CRj, CRi", "2+a", FLAG_NOT_IN_DELAY_SLOT, {}))
// (COPLD, "COPLD #u4, #CC, Rj, CRi", "1+2a", FLAG_NOT_IN_DELAY_SLOT, {}))
// (COPST, "COPST #u4, #CC, CRj, Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT, {}))
// (COPSV, "COPSV #u4, #CC, CRj, Ri", "1+2a", FLAG_NOT_IN_DELAY_SLOT, {}))
FR_GEN_CODE(COP1, "COPOP/COPLD/COPST/COPSV", "x", FLAG_NOT_IN_DELAY_SLOT,
({
    qemu_log_mask(LOG_UNIMP, "%s: COPOP/COPLD/COPST/COPSV is not supported\n", __func__);
}))

FR_GEN_CODE(SRCH0, "SRCH0 Ri", 1, FLAG_NONE,
({
    tcg_gen_not_tl(tcg_Ri, tcg_Ri);
    tcg_gen_clzi_tl(tcg_Ri, tcg_Ri, 32);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(SRCH1, "SRCH1 Ri", 1, FLAG_NONE,
({
    tcg_gen_clzi_tl(tcg_Ri, tcg_Ri, 32);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(SRCHC, "SRCHC Ri", 1, FLAG_NONE,
({
    tcg_gen_clrsb_tl(tcg_Ri, tcg_Ri);
    tcg_gen_addi_tl(tcg_Ri, tcg_Ri, 1);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(NOP, "NOP", 1, FLAG_NONE,
({
    // Do nothing
}))

FR_GEN_CODE(ANDCCR, "ANDCCR #u8", "c", (FLAG_UPDATE_NZVC|FLAG_SI),
({
    uint32_t si = ((a->u8) | (~(R_PS_CCR_I|R_PS_CCR_S)));
    tcg_gen_andi_tl(tcg_PS, tcg_PS, si);

    // don't care about the NZCV bit in R_PS, will be sync'd
    tcg_gen_andi_tl(cpu_PS_N, cpu_PS_N, ((uint32_t)a->u8 >> R_PS_CCR_N_BIT)&0x1);
    tcg_gen_andi_tl(cpu_PS_Z, cpu_PS_Z, ((uint32_t)a->u8 >> R_PS_CCR_Z_BIT)&0x1);
    tcg_gen_andi_tl(cpu_PS_V, cpu_PS_V, ((uint32_t)a->u8 >> R_PS_CCR_V_BIT)&0x1);
    tcg_gen_andi_tl(cpu_PS_C, cpu_PS_C, ((uint32_t)a->u8 >> R_PS_CCR_C_BIT)&0x1);

    if (((a->u8) & R_PS_CCR_S) == 0) { // clear R_PS_CCR_S, S flag may changed
        WRITTING_SP(R_SSP);
    }
}))

FR_GEN_CODE(ORCCR, "ORCCR #u8", "c", (FLAG_UPDATE_NZVC|FLAG_SI),
({
    uint32_t si = ((a->u8) & (R_PS_CCR_I|R_PS_CCR_S));
    tcg_gen_ori_tl(tcg_PS, tcg_PS, si);

    // don't care about the NZCV bit in R_PS, will be sync'd
    tcg_gen_ori_tl(cpu_PS_N, cpu_PS_N, ((uint32_t)a->u8 >> R_PS_CCR_N_BIT)&0x1);
    tcg_gen_ori_tl(cpu_PS_Z, cpu_PS_Z, ((uint32_t)a->u8 >> R_PS_CCR_Z_BIT)&0x1);
    tcg_gen_ori_tl(cpu_PS_V, cpu_PS_V, ((uint32_t)a->u8 >> R_PS_CCR_V_BIT)&0x1);
    tcg_gen_ori_tl(cpu_PS_C, cpu_PS_C, ((uint32_t)a->u8 >> R_PS_CCR_C_BIT)&0x1);

    if ((a->u8) & R_PS_CCR_S) { // set R_PS_CCR_S, S flag may changed
        WRITTING_SP(R_SSP);
    }
}))

FR_GEN_CODE(STILM, "STILM #u8", "1", FLAG_NONE,
({
    // Clear the bit ILM0-3
    tcg_gen_andi_tl(tcg_PS, tcg_PS, (uint32_t)(~(R_PS_ILM0|R_PS_ILM1|R_PS_ILM2|R_PS_ILM3)));
    tcg_gen_ori_tl(tcg_PS, tcg_PS, (a->u8)<<R_PS_ILM0_BIT);
}))

FR_GEN_CODE(ADDSP, "ADDSP #s10", "1", FLAG_NONE,
({
    tcg_gen_addi_tl(tcg_R(R_15), tcg_R(R_15), a->s10);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(EXTSB, "EXTSB Ri", "1", FLAG_NONE,
({
    tcg_gen_ext8s_tl(tcg_Ri, tcg_Ri);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(EXTUB, "EXTUB Ri", "1", FLAG_NONE,
({
    tcg_gen_ext8u_tl(tcg_Ri, tcg_Ri);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(EXTSH, "EXTSH Ri", "1", FLAG_NONE,
({
    tcg_gen_ext16s_tl(tcg_Ri, tcg_Ri);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(EXTUH, "EXTUH Ri", "1", FLAG_NONE,
({
    tcg_gen_ext16u_tl(tcg_Ri, tcg_Ri);
    WRITTING_R15(a->ri);
}))

FR_GEN_CODE(LDM0, "LDM0 (reglist)", "1/1+b+a(n-1)", FLAG_NOT_IN_DELAY_SLOT,
({  // LDM0: bit 0 is R0, bit 7 is R7
    int reg;
    TCGv t4 = tcg_const_local_tl(4);
    DECLARE_BITMAP(ldm, 8) = {a->reglist};
    for (reg=0; reg<=7; reg++) {
        if (test_bit(reg, ldm)) {
            tcg_gen_qemu_ld32u(tcg_R(reg), tcg_R(R_15), 0);
            tcg_gen_add_tl(tcg_R(R_15), tcg_R(R_15), t4);
        }
    }
    tcg_temp_free(t4);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(LDM1, "LDM1 (reglist)", "1/1+b+a(n-1)", FLAG_NOT_IN_DELAY_SLOT,
({  // LDM1: bit 0 is R8, bit 7 is R15
    int reg;
    TCGv t4 = tcg_const_local_tl(4);
    DECLARE_BITMAP(ldm, 8) = {a->reglist};
    for (reg=8; reg<=15; reg++) {
        if (test_bit(reg-8, ldm)) {
            tcg_gen_qemu_ld32u(tcg_R(reg), tcg_R(R_15), 0);
            if (reg != R_15) {
                tcg_gen_add_tl(tcg_R(R_15), tcg_R(R_15), t4);
            }
        }
    }
    tcg_temp_free(t4);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(STM0, "STM0 (reglist)", "1+an", FLAG_NOT_IN_DELAY_SLOT,
({  // STM0: bit 0 is R7, bit 7 is R0
    int reg;
    TCGv t4 = tcg_const_local_tl(4);
    DECLARE_BITMAP(stm, 8) = {a->reglist};
    for (reg=7; reg>=0; reg--) {
        if (test_bit((7-reg), stm)) {
            tcg_gen_sub_tl(tcg_R(R_15), tcg_R(R_15), t4);
            tcg_gen_qemu_st32(tcg_R(reg), tcg_R(R_15), 0);
        }
    }
    tcg_temp_free(t4);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(STM1, "STM1 (reglist)", "1+an", FLAG_NOT_IN_DELAY_SLOT,
({  // STM1: bit 0 is R15, bit 7 is R8
    int reg;
    TCGv t4 = tcg_const_local_tl(4);
    DECLARE_BITMAP(stm, 8) = {a->reglist};
    for (reg=15; reg>7; reg--) {
        if (test_bit((15-reg), stm)) {
            if (reg == R_15) {
                TCGv tmp = tcg_temp_local_new();
                tcg_gen_mov_tl(tmp, tcg_R(R_15));
                tcg_gen_sub_tl(tcg_R(R_15), tcg_R(R_15), t4);
                tcg_gen_qemu_st32(tmp, tcg_R(R_15), 0);

                tcg_temp_free(tmp);
            }
            else {
                tcg_gen_sub_tl(tcg_R(R_15), tcg_R(R_15), t4);
                tcg_gen_qemu_st32(tcg_R(reg), tcg_R(R_15), 0);
            }
        }
    }
    tcg_temp_free(t4);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(ENTER, "ENTER #u10", "1+a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_subi_tl(tmp, tcg_R(R_15), 4);
    tcg_gen_qemu_st32(tcg_R(R_14), tmp, 0);
    tcg_gen_mov_tl(tcg_R(R_14), tmp);
    tcg_gen_subi_tl(tcg_R(R_15), tcg_R(R_15), a->u10);
    tcg_temp_free(tmp);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(LEAVE, "LEAVE", "b", FLAG_NONE,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_addi_tl(tcg_R(R_15), tcg_R(R_14), 4);
    tcg_gen_subi_tl(tmp, tcg_R(R_15), 4);
    tcg_gen_qemu_ld32u(tcg_R(R_14), tmp, 0);
    tcg_temp_free(tmp);
    WRITTING_R15(R_15);
}))

FR_GEN_CODE(XCHB, "XCHB @Rj, Ri", "2a", FLAG_NOT_IN_DELAY_SLOT,
({
    TCGv tmp = tcg_temp_local_new();
    tcg_gen_mov_tl(tmp, tcg_Ri);
    tcg_gen_qemu_ld8u(tcg_Ri, tcg_Rj, 0);
    tcg_gen_ext8u_tl(tcg_Ri, tcg_Ri);
    tcg_gen_qemu_st8(tmp, tcg_Rj, 0);
    tcg_temp_free(tmp);
    WRITTING_R15(a->ri);
}))
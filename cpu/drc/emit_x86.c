/*
 * Basic macros to emit x86 instructions and some utils
 * Copyright (C) 2008,2009,2010 notaz
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 *
 * note:
 *  temp registers must be eax-edx due to use of SETcc and r/w 8/16.
 * note about silly things like emith_eor_r_r_r:
 *  these are here because the compiler was designed
 *  for ARM as it's primary target.
 */
#include <stdarg.h>

enum { xAX = 0, xCX, xDX, xBX, xSP, xBP, xSI, xDI };

#define HOST_REGS   8
#define CONTEXT_REG xBP
#define RET_REG     xAX

#define ICOND_JO  0x00
#define ICOND_JNO 0x01
#define ICOND_JB  0x02
#define ICOND_JAE 0x03
#define ICOND_JE  0x04
#define ICOND_JNE 0x05
#define ICOND_JBE 0x06
#define ICOND_JA  0x07
#define ICOND_JS  0x08
#define ICOND_JNS 0x09
#define ICOND_JL  0x0c
#define ICOND_JGE 0x0d
#define ICOND_JLE 0x0e
#define ICOND_JG  0x0f

#define IOP_JMP   0xeb

// unified conditions (we just use rel8 jump instructions for x86)
#define DCOND_EQ ICOND_JE
#define DCOND_NE ICOND_JNE
#define DCOND_MI ICOND_JS      // MInus
#define DCOND_PL ICOND_JNS     // PLus or zero
#define DCOND_HI ICOND_JA      // higher (unsigned)
#define DCOND_HS ICOND_JAE     // higher || same (unsigned)
#define DCOND_LO ICOND_JB      // lower (unsigned)
#define DCOND_LS ICOND_JBE     // lower || same (unsigned)
#define DCOND_GE ICOND_JGE     // greater || equal (signed)
#define DCOND_GT ICOND_JG      // greater (signed)
#define DCOND_LE ICOND_JLE     // less || equal (signed)
#define DCOND_LT ICOND_JL      // less (signed)
#define DCOND_VS ICOND_JO      // oVerflow Set
#define DCOND_VC ICOND_JNO     // oVerflow Clear

#define DCOND_CS ICOND_JB      // carry set
#define DCOND_CC ICOND_JAE     // carry clear

#define EMIT_PTR(ptr, val, type) \
	*(type *)(ptr) = val

#define EMIT(val, type) do { \
	EMIT_PTR(tcache_ptr, val, type); \
	tcache_ptr += sizeof(type); \
} while (0)

#define EMIT_OP(op) do { \
	COUNT_OP; \
	EMIT(op, u8); \
} while (0)

#define EMIT_MODRM(mod, r, rm) do { \
	assert((mod) < 4u); \
	assert((r) < 8u); \
	assert((rm) < 8u); \
	EMIT(((mod)<<6) | ((r)<<3) | (rm), u8); \
} while (0)

#define EMIT_SIB(scale, index, base) do { \
	assert((scale) < 4u); \
	assert((index) < 8u); \
	assert((base) < 8u); \
	EMIT(((scale)<<6) | ((index)<<3) | (base), u8); \
} while (0)

#define EMIT_SIB64(scale, index, base) \
	EMIT_SIB(scale, (index) & ~8u, (base) & ~8u)

#define EMIT_REX(w,r,x,b) \
	EMIT(0x40 | ((w)<<3) | ((r)<<2) | ((x)<<1) | (b), u8)

#define EMIT_OP_MODRM(op,mod,r,rm) do { \
	EMIT_OP(op); \
	EMIT_MODRM(mod, (r), rm); \
} while (0)

// 64bit friendly, rm when everything is converted
#define EMIT_OP_MODRM64(op, mod, r, rm) \
	EMIT_OP_MODRM(op, mod, (r) & ~8u, (rm) & ~8u)

#define JMP8_POS(ptr) \
	ptr = tcache_ptr; \
	tcache_ptr += 2

#define JMP8_EMIT(op, ptr) \
	EMIT_PTR(ptr, 0x70|(op), u8); \
	EMIT_PTR(ptr + 1, (tcache_ptr - (ptr+2)), u8)

#define JMP8_EMIT_NC(ptr) \
	EMIT_PTR(ptr, IOP_JMP, u8); \
	EMIT_PTR(ptr + 1, (tcache_ptr - (ptr+2)), u8)

// _r_r
#define emith_move_r_r(dst, src) \
	EMIT_OP_MODRM(0x8b, 3, dst, src)

#define emith_move_r_r_ptr(dst, src) do { \
	EMIT_REX_IF(1, dst, src); \
	EMIT_OP_MODRM64(0x8b, 3, dst, src); \
} while (0)

#define emith_add_r_r(d, s) \
	EMIT_OP_MODRM(0x01, 3, s, d)

#define emith_add_r_r_ptr(d, s) do { \
	EMIT_REX_IF(1, s, d); \
	EMIT_OP_MODRM64(0x01, 3, s, d); \
} while (0)

#define emith_sub_r_r(d, s) \
	EMIT_OP_MODRM(0x29, 3, s, d)

#define emith_adc_r_r(d, s) \
	EMIT_OP_MODRM(0x11, 3, s, d)

#define emith_sbc_r_r(d, s) \
	EMIT_OP_MODRM(0x19, 3, s, d) /* SBB */

#define emith_or_r_r(d, s) \
	EMIT_OP_MODRM(0x09, 3, s, d)

#define emith_and_r_r(d, s) \
	EMIT_OP_MODRM(0x21, 3, s, d)

#define emith_eor_r_r(d, s) \
	EMIT_OP_MODRM(0x31, 3, s, d) /* XOR */

#define emith_tst_r_r(d, s) \
	EMIT_OP_MODRM(0x85, 3, s, d) /* TEST */

#define emith_tst_r_r_ptr(d, s) do { \
	EMIT_REX_IF(1, s, d); \
	EMIT_OP_MODRM64(0x85, 3, s, d); /* TEST */ \
} while (0)

#define emith_cmp_r_r(d, s) \
	EMIT_OP_MODRM(0x39, 3, s, d)

// fake teq - test equivalence - get_flags(d ^ s)
#define emith_teq_r_r(d, s) do { \
	emith_push(d); \
	emith_eor_r_r(d, s); \
	emith_pop(d); \
} while (0)

#define emith_mvn_r_r(d, s) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	EMIT_OP_MODRM(0xf7, 3, 2, d); /* NOT d */ \
} while (0)

#define emith_negc_r_r(d, s) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_move_r_imm(tmp_, 0); \
	emith_sbc_r_r(tmp_, s); \
	emith_move_r_r(d, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

#define emith_neg_r_r(d, s) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	EMIT_OP_MODRM(0xf7, 3, 3, d); /* NEG d */ \
} while (0)

// _r_r_r
#define emith_add_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_add_r_r(d, s2); \
	} else if (d == s2) { \
		emith_add_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_add_r_r(d, s2); \
	} \
} while (0)

#define emith_add_r_r_r_ptr(d, s1, s2) do { \
	if (d == s1) { \
		emith_add_r_r_ptr(d, s2); \
	} else if (d == s2) { \
		emith_add_r_r_ptr(d, s1); \
	} else { \
		emith_move_r_r_ptr(d, s1); \
		emith_add_r_r_ptr(d, s2); \
	} \
} while (0)

#define emith_sub_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_sub_r_r(d, s2); \
	} else if (d == s2) { \
		emith_sub_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_sub_r_r(d, s2); \
	} \
} while (0)

#define emith_adc_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_adc_r_r(d, s2); \
	} else if (d == s2) { \
		emith_adc_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_adc_r_r(d, s2); \
	} \
} while (0)

#define emith_sbc_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_sbc_r_r(d, s2); \
	} else if (d == s2) { \
		emith_sbc_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_sbc_r_r(d, s2); \
	} \
} while (0)

#define emith_and_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_and_r_r(d, s2); \
	} else if (d == s2) { \
		emith_and_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_and_r_r(d, s2); \
	} \
} while (0)

#define emith_or_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_or_r_r(d, s2); \
	} else if (d == s2) { \
		emith_or_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_or_r_r(d, s2); \
	} \
} while (0)

#define emith_eor_r_r_r(d, s1, s2) do { \
	if (d == s1) { \
		emith_eor_r_r(d, s2); \
	} else if (d == s2) { \
		emith_eor_r_r(d, s1); \
	} else { \
		emith_move_r_r(d, s1); \
		emith_eor_r_r(d, s2); \
	} \
} while (0)

// _r_r_r_shift
#define emith_add_r_r_r_lsl(d, s1, s2, lslimm) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_lsl(tmp_, s2, lslimm); \
	emith_add_r_r_r(d, s1, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

#define emith_add_r_r_r_lsl_ptr(d, s1, s2, lslimm) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_lsl(tmp_, s2, lslimm); \
	emith_add_r_r_r_ptr(d, s1, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

#define emith_add_r_r_r_lsr(d, s1, s2, lsrimm) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_lsr(tmp_, s2, lsrimm); \
	emith_add_r_r_r(d, s1, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

#define emith_sub_r_r_r_lsl(d, s1, s2, lslimm) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_lsl(tmp_, s2, lslimm); \
	emith_sub_r_r_r(d, s1, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

#define emith_or_r_r_r_lsl(d, s1, s2, lslimm) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_lsl(tmp_, s2, lslimm); \
	emith_or_r_r_r(d, s1, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

// _r_r_shift
#define emith_or_r_r_lsl(d, s, lslimm) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_lsl(tmp_, s, lslimm); \
	emith_or_r_r(d, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

// d != s
#define emith_eor_r_r_lsr(d, s, lsrimm) do { \
	emith_push(s); \
	emith_lsr(s, s, lsrimm); \
	emith_eor_r_r(d, s); \
	emith_pop(s); \
} while (0)

// _r_imm
#define emith_move_r_imm(r, imm) do { \
	EMIT_OP(0xb8 + (r)); \
	EMIT(imm, u32); \
} while (0)

#define emith_move_r_ptr_imm(r, imm) do { \
	if ((uint64_t)(imm) <= UINT32_MAX) \
		emith_move_r_imm(r, (uintptr_t)(imm)); \
	else { \
		EMIT_REX_IF(1, 0, r); \
		EMIT_OP(0xb8 + (r)); \
		EMIT((uint64_t)(imm), uint64_t); \
	} \
} while (0)

#define emith_move_r_imm_s8(r, imm) \
	emith_move_r_imm(r, (u32)(signed int)(signed char)(imm))

#define emith_arith_r_imm(op, r, imm) do { \
	EMIT_OP_MODRM(0x81, 3, op, r); \
	EMIT(imm, u32); \
} while (0)

#define emith_add_r_imm(r, imm) \
	emith_arith_r_imm(0, r, imm)

#define emith_or_r_imm(r, imm) \
	emith_arith_r_imm(1, r, imm)

#define emith_adc_r_imm(r, imm) \
	emith_arith_r_imm(2, r, imm)

#define emith_sbc_r_imm(r, imm) \
	emith_arith_r_imm(3, r, imm) // sbb

#define emith_and_r_imm(r, imm) \
	emith_arith_r_imm(4, r, imm)

#define emith_sub_r_imm(r, imm) \
	emith_arith_r_imm(5, r, imm)

#define emith_eor_r_imm(r, imm) \
	emith_arith_r_imm(6, r, imm)

#define emith_cmp_r_imm(r, imm) \
	emith_arith_r_imm(7, r, imm)

#define emith_tst_r_imm(r, imm) do { \
	EMIT_OP_MODRM(0xf7, 3, 0, r); \
	EMIT(imm, u32); \
} while (0)

// fake
#define emith_bic_r_imm(r, imm) \
	emith_arith_r_imm(4, r, ~(imm))

// fake conditionals (using SJMP instead)
#define emith_move_r_imm_c(cond, r, imm) \
	emith_move_r_imm(r, imm);
#define emith_add_r_imm_c(cond, r, imm) \
	emith_add_r_imm(r, imm);
#define emith_sub_r_imm_c(cond, r, imm) \
	emith_sub_r_imm(r, imm);
#define emith_or_r_imm_c(cond, r, imm) \
	emith_or_r_imm(r, imm)
#define emith_eor_r_imm_c(cond, r, imm) \
	emith_eor_r_imm(r, imm)
#define emith_bic_r_imm_c(cond, r, imm) \
	emith_bic_r_imm(r, imm)
#define emith_tst_r_imm_c(cond, r, imm) \
	emith_tst_r_imm(r, imm)
#define emith_move_r_r_ptr_c(cond, d, s) \
	emith_move_r_r_ptr(d, s)
#define emith_ror_c(cond, d, s, cnt) \
	emith_ror(d, s, cnt)
#define emith_and_r_r_c(cond, d, s) \
	emith_and_r_r(d, s);
#define emith_add_r_r_imm_c(cond, d, s, imm) \
	emith_add_r_r_imm(d, s, imm);
#define emith_sub_r_r_imm_c(cond, d, s, imm) \
	emith_sub_r_r_imm(d, s, imm);

#define emith_read8_r_r_r_c(cond, r, rs, rm) \
	emith_read8_r_r_r(r, rs, rm)
#define emith_read8s_r_r_r_c(cond, r, rs, rm) \
	emith_read8s_r_r_r(r, rs, rm)
#define emith_read16_r_r_r_c(cond, r, rs, rm) \
	emith_read16_r_r_r(r, rs, rm)
#define emith_read16s_r_r_r_c(cond, r, rs, rm) \
	emith_read16s_r_r_r(r, rs, rm)
#define emith_read_r_r_r_c(cond, r, rs, rm) \
	emith_read_r_r_r(r, rs, rm)

#define emith_read_r_r_offs_c(cond, r, rs, offs) \
	emith_read_r_r_offs(r, rs, offs)
#define emith_read_r_r_offs_ptr_c(cond, r, rs, offs) \
	emith_read_r_r_offs_ptr(r, rs, offs)
#define emith_write_r_r_offs_c(cond, r, rs, offs) \
	emith_write_r_r_offs(r, rs, offs)
#define emith_write_r_r_offs_ptr_c(cond, r, rs, offs) \
	emith_write_r_r_offs_ptr(r, rs, offs)
#define emith_read8_r_r_offs_c(cond, r, rs, offs) \
	emith_read8_r_r_offs(r, rs, offs)
#define emith_write8_r_r_offs_c(cond, r, rs, offs) \
	emith_write8_r_r_offs(r, rs, offs)
#define emith_read16_r_r_offs_c(cond, r, rs, offs) \
	emith_read16_r_r_offs(r, rs, offs)
#define emith_write16_r_r_offs_c(cond, r, rs, offs) \
	emith_write16_r_r_offs(r, rs, offs)
#define emith_jump_reg_c(cond, r) \
	emith_jump_reg(r)
#define emith_jump_ctx_c(cond, offs) \
	emith_jump_ctx(offs)
#define emith_ret_c(cond) \
	emith_ret()

// _r_r_imm - use lea
#define emith_add_r_r_imm(d, s, imm) do { \
	assert(s != xSP); \
	EMIT_OP_MODRM(0x8d, 2, d, s); /* lea */ \
	EMIT(imm, s32); \
} while (0)

#define emith_add_r_r_ptr_imm(d, s, imm) do { \
	if ((s) != xSP) { \
		EMIT_REX_IF(1, d, s); \
		EMIT_OP_MODRM64(0x8d, 2, d, s); /* lea */ \
	} \
	else { \
		if (d != s) \
			emith_move_r_r_ptr(d, s); \
		EMIT_REX_IF(1, 0, d); \
		EMIT_OP_MODRM64(0x81, 3, 0, d); /* add */ \
	} \
	EMIT(imm, s32); \
} while (0)

#define emith_sub_r_r_imm(d, s, imm) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	if ((s32)(imm) != 0) \
		emith_sub_r_imm(d, imm); \
} while (0)

#define emith_and_r_r_imm(d, s, imm) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	if ((s32)(imm) != -1) \
		emith_and_r_imm(d, imm); \
} while (0)

#define emith_or_r_r_imm(d, s, imm) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	if ((s32)(imm) != 0) \
		emith_or_r_imm(d, imm); \
} while (0)

#define emith_eor_r_r_imm(d, s, imm) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	if ((s32)(imm) != 0) \
		emith_eor_r_imm(d, imm); \
} while (0)

// shift
#define emith_shift(op, d, s, cnt) do { \
	if (d != s) \
		emith_move_r_r(d, s); \
	EMIT_OP_MODRM(0xc1, 3, op, d); \
	EMIT(cnt, u8); \
} while (0)

#define emith_lsl(d, s, cnt) \
	emith_shift(4, d, s, cnt)

#define emith_lsr(d, s, cnt) \
	emith_shift(5, d, s, cnt)

#define emith_asr(d, s, cnt) \
	emith_shift(7, d, s, cnt)

#define emith_rol(d, s, cnt) \
	emith_shift(0, d, s, cnt)

#define emith_ror(d, s, cnt) \
	emith_shift(1, d, s, cnt)

#define emith_rolc(r) \
	EMIT_OP_MODRM(0xd1, 3, 2, r)

#define emith_rorc(r) \
	EMIT_OP_MODRM(0xd1, 3, 3, r)

// misc
#define emith_push(r) \
	EMIT_OP(0x50 + (r))

#define emith_push_imm(imm) do { \
	EMIT_OP(0x68); \
	EMIT(imm, u32); \
} while (0)

#define emith_pop(r) \
	EMIT_OP(0x58 + (r))

#define emith_neg_r(r) \
	EMIT_OP_MODRM(0xf7, 3, 3, r)

#define emith_clear_msb(d, s, count) do { \
	u32 t = (u32)-1; \
	t >>= count; \
	if (d != s) \
		emith_move_r_r(d, s); \
	emith_and_r_imm(d, t); \
} while (0)

#define emith_clear_msb_c(cond, d, s, count) do { \
	(void)(cond); \
	emith_clear_msb(d, s, count); \
} while (0)

#define emith_sext(d, s, bits) do { \
	emith_lsl(d, s, 32 - (bits)); \
	emith_asr(d, d, 32 - (bits)); \
} while (0)

#define emith_setc(r) do { \
	assert(is_abcdx(r)); \
	EMIT_OP(0x0f); \
	EMIT_OP_MODRM(0x92, 3, 0, r); /* SETC r */ \
} while (0)

// XXX: stupid mess
#define emith_mul_(op, dlo, dhi, s1, s2) do { \
	int rmr; \
	if (dlo != xAX && dhi != xAX) \
		emith_push(xAX); \
	if (dlo != xDX && dhi != xDX) \
		emith_push(xDX); \
	if ((s1) == xAX) \
		rmr = s2; \
	else if ((s2) == xAX) \
		rmr = s1; \
	else { \
		emith_move_r_r(xAX, s1); \
		rmr = s2; \
	} \
	EMIT_OP_MODRM(0xf7, 3, op, rmr); /* xMUL rmr */ \
	if (dlo != xAX) \
		EMIT_OP(0x90 + (dlo)); /* XCHG eax, dlo */ \
	if (dhi != xDX && dhi != -1 && !(dhi == xAX && dlo == xDX)) \
		emith_move_r_r(dhi, (dlo == xDX ? xAX : xDX)); \
	if (dlo != xDX && dhi != xDX) \
		emith_pop(xDX); \
	if (dlo != xAX && dhi != xAX) \
		emith_pop(xAX); \
} while (0)

#define emith_mul_u64(dlo, dhi, s1, s2) \
	emith_mul_(4, dlo, dhi, s1, s2) /* MUL */

#define emith_mul_s64(dlo, dhi, s1, s2) \
	emith_mul_(5, dlo, dhi, s1, s2) /* IMUL */

#define emith_mul(d, s1, s2) \
	emith_mul_(4, d, -1, s1, s2)

// (dlo,dhi) += signed(s1) * signed(s2)
#define emith_mula_s64(dlo, dhi, s1, s2) do { \
	emith_push(dhi); \
	emith_push(dlo); \
	emith_mul_(5, dlo, dhi, s1, s2); \
	EMIT_OP_MODRM(0x03, 0, dlo, 4); \
	EMIT_SIB(0, 4, 4); /* add dlo, [xsp] */ \
	EMIT_OP_MODRM(0x13, 1, dhi, 4); \
	EMIT_SIB(0, 4, 4); \
	EMIT(sizeof(void *), u8); /* adc dhi, [xsp+{4,8}] */ \
	emith_add_r_r_ptr_imm(xSP, xSP, sizeof(void *) * 2); \
} while (0)

// "flag" instructions are the same
#define emith_adcf_r_imm emith_adc_r_imm
#define emith_subf_r_imm emith_sub_r_imm
#define emith_addf_r_r   emith_add_r_r
#define emith_subf_r_r   emith_sub_r_r
#define emith_adcf_r_r   emith_adc_r_r
#define emith_sbcf_r_r   emith_sbc_r_r
#define emith_eorf_r_r   emith_eor_r_r
#define emith_negcf_r_r  emith_negc_r_r

#define emith_subf_r_r_imm emith_sub_r_r_imm
#define emith_addf_r_r_r emith_add_r_r_r
#define emith_subf_r_r_r emith_sub_r_r_r
#define emith_adcf_r_r_r emith_adc_r_r_r
#define emith_sbcf_r_r_r emith_sbc_r_r_r
#define emith_eorf_r_r_r emith_eor_r_r_r
#define emith_addf_r_r_r_lsr emith_add_r_r_r_lsr

#define emith_lslf  emith_lsl
#define emith_lsrf  emith_lsr
#define emith_asrf  emith_asr
#define emith_rolf  emith_rol
#define emith_rorf  emith_ror
#define emith_rolcf emith_rolc
#define emith_rorcf emith_rorc

#define emith_deref_op(op, r, rs, offs) do { \
	/* mov r <-> [ebp+#offs] */ \
	if (abs(offs) >= 0x80) { \
		EMIT_OP_MODRM64(op, 2, r, rs); \
		EMIT(offs, u32); \
	} else { \
		EMIT_OP_MODRM64(op, 1, r, rs); \
		EMIT((u8)offs, u8); \
	} \
} while (0)

#define is_abcdx(r) (xAX <= (r) && (r) <= xDX)

#define emith_read_r_r_offs(r, rs, offs) \
	emith_deref_op(0x8b, r, rs, offs)
#define emith_read_r_r_offs_ptr(r, rs, offs) \
	EMIT_REX_IF(1, r, rs); \
	emith_deref_op(0x8b, r, rs, offs)

#define emith_write_r_r_offs(r, rs, offs) \
	emith_deref_op(0x89, r, rs, offs)
#define emith_write_r_r_offs_ptr(r, rs, offs) \
	EMIT_REX_IF(1, r, rs); \
	emith_deref_op(0x89, r, rs, offs)

#define emith_read8_r_r_offs(r, rs, offs) do { \
	EMIT(0x0f, u8); \
	emith_deref_op(0xb6, r, rs, offs); \
} while (0)

#define emith_read8s_r_r_offs(r, rs, offs) do { \
	EMIT(0x0f, u8); \
	emith_deref_op(0xbe, r, rs, offs); \
} while (0)

// note: don't use prefixes on this
#define emith_write8_r_r_offs(r, rs, offs) do {\
	int r_ = r; \
	if (!is_abcdx(r)) { \
		r_ = rcache_get_tmp(); \
		emith_move_r_r(r_, r); \
	} \
	emith_deref_op(0x88, r_, rs, offs); \
	if ((r) != r_) \
		rcache_free_tmp(r_); \
} while (0)

#define emith_read16_r_r_offs(r, rs, offs) do { \
	EMIT(0x0f, u8); \
	emith_deref_op(0xb7, r, rs, offs); \
} while (0)

#define emith_read16s_r_r_offs(r, rs, offs) do { \
	EMIT(0x0f, u8); \
	emith_deref_op(0xbf, r, rs, offs); \
} while (0)

#define emith_write16_r_r_offs(r, rs, offs) do { \
	EMIT(0x66, u8); \
	emith_write_r_r_offs(r, rs, offs); \
} while (0)

#define emith_read8_r_r_r(r, rs, rm) do { \
	EMIT(0x0f, u8); \
	EMIT_OP_MODRM(0xb6, 0, r, 4); \
	EMIT_SIB(0, rs, rm); /* mov r, [rm + rs * 1] */ \
} while (0)

#define emith_read8s_r_r_r(r, rs, rm) do { \
	EMIT(0x0f, u8); \
	EMIT_OP_MODRM(0xbe, 0, r, 4); \
	EMIT_SIB(0, rs, rm); /* mov r, [rm + rs * 1] */ \
} while (0)

#define emith_read16_r_r_r(r, rs, rm) do { \
	EMIT(0x0f, u8); \
	EMIT_OP_MODRM(0xb7, 0, r, 4); \
	EMIT_SIB(0, rs, rm); /* mov r, [rm + rs * 1] */ \
} while (0)

#define emith_read16s_r_r_r(r, rs, rm) do { \
	EMIT(0x0f, u8); \
	EMIT_OP_MODRM(0xbf, 0, r, 4); \
	EMIT_SIB(0, rs, rm); /* mov r, [rm + rs * 1] */ \
} while (0)

#define emith_read_r_r_r(r, rs, rm) do { \
	EMIT_OP_MODRM(0x8b, 0, r, 4); \
	EMIT_SIB(0, rs, rm); /* mov r, [rm + rs * 1] */ \
} while (0)
#define emith_read_r_r_r_wb(r, rs, rm) do { \
	emith_read_r_r_r(r, rs, rm); \
	emith_add_r_r_ptr(rs, rm); \
} while (0)

#define emith_write_r_r_r(r, rs, rm) do { \
	EMIT_OP_MODRM(0x89, 0, r, 4); \
	EMIT_SIB(0, rs, rm); /* mov [rm + rs * 1], r */ \
} while (0)
#define emith_write_r_r_r_wb(r, rs, rm) do { \
	emith_write_r_r_r(r, rs, rm); \
	emith_add_r_r_ptr(rs, rm); \
} while (0)


#define emith_ctx_read(r, offs) \
	emith_read_r_r_offs(r, CONTEXT_REG, offs)
#define emith_ctx_read_c(cond, r, offs) \
	emith_ctx_read(r, offs)

#define emith_ctx_read_ptr(r, offs) do { \
	EMIT_REX_IF(1, r, CONTEXT_REG); \
	emith_deref_op(0x8b, r, CONTEXT_REG, offs); \
} while (0)

#define emith_ctx_write(r, offs) \
	emith_write_r_r_offs(r, CONTEXT_REG, offs)

#define emith_ctx_read_multiple(r, offs, cnt, tmpr) do { \
	int r_ = r, offs_ = offs, cnt_ = cnt;     \
	for (; cnt_ > 0; r_++, offs_ += 4, cnt_--) \
		emith_ctx_read(r_, offs_);        \
} while (0)

#define emith_ctx_write_multiple(r, offs, cnt, tmpr) do { \
	int r_ = r, offs_ = offs, cnt_ = cnt;     \
	for (; cnt_ > 0; r_++, offs_ += 4, cnt_--) \
		emith_ctx_write(r_, offs_);       \
} while (0)

// assumes EBX is free
#define emith_ret_to_ctx(offs) do { \
	emith_pop(xBX); \
	emith_ctx_write(xBX, offs); \
} while (0)

#define emith_jump(ptr) do { \
	u32 disp = (u8 *)(ptr) - ((u8 *)tcache_ptr + 5); \
	EMIT_OP(0xe9); \
	EMIT(disp, u32); \
} while (0)

#define emith_jump_patchable(target) \
	emith_jump(target)

#define emith_jump_cond(cond, ptr) do { \
	u32 disp = (u8 *)(ptr) - ((u8 *)tcache_ptr + 6); \
	EMIT(0x0f, u8); \
	EMIT_OP(0x80 | (cond)); \
	EMIT(disp, u32); \
} while (0)

#define emith_jump_cond_patchable(cond, target) \
	emith_jump_cond(cond, target)

#define emith_jump_patch(ptr, target) do { \
	u32 disp_ = (u8 *)(target) - ((u8 *)(ptr) + 4); \
	u32 offs_ = (*(u8 *)(ptr) == 0x0f) ? 2 : 1; \
	EMIT_PTR((u8 *)(ptr) + offs_, disp_ - offs_, u32); \
} while (0)

#define emith_jump_at(ptr, target) do { \
	u32 disp_ = (u8 *)(target) - ((u8 *)(ptr) + 5); \
	EMIT_PTR(ptr, 0xe9, u8); \
	EMIT_PTR((u8 *)(ptr) + 1, disp_, u32); \
} while (0)

#define emith_call(ptr) do { \
	u32 disp = (u8 *)(ptr) - ((u8 *)tcache_ptr + 5); \
	EMIT_OP(0xe8); \
	EMIT(disp, u32); \
} while (0)

#define emith_call_cond(cond, ptr) \
	emith_call(ptr)

#define emith_call_reg(r) \
	EMIT_OP_MODRM(0xff, 3, 2, r)

#define emith_call_ctx(offs) do { \
	EMIT_OP_MODRM(0xff, 2, 2, CONTEXT_REG); \
	EMIT(offs, u32); \
} while (0)

#define emith_call_link(r, target) do { \
	EMIT_OP(0xe8); \
	EMIT(0, u32); /* call pc+0 */ \
	emith_pop(r); \
	emith_add_r_r_ptr_imm(r, r, 13); \
	emith_jump(target); \
} while (0)

#define emith_ret() \
	EMIT_OP(0xc3)

#define emith_jump_reg(r) \
	EMIT_OP_MODRM(0xff, 3, 4, r)

#define emith_jump_ctx(offs) do { \
	EMIT_OP_MODRM(0xff, 2, 4, CONTEXT_REG); \
	EMIT(offs, u32); \
} while (0)

#define emith_push_ret(r) do { \
	int r_ = (r >= 0 ? r : xSI); \
	emith_push(r_); /* always push to align */ \
} while (0)

#define emith_pop_and_ret(r) do { \
	int r_ = (r >= 0 ? r : xSI); \
	emith_pop(r_); \
	emith_ret(); \
} while (0)

#define EMITH_JMP_START(cond) { \
	u8 *cond_ptr; \
	JMP8_POS(cond_ptr)

#define EMITH_JMP_END(cond) \
	JMP8_EMIT(cond, cond_ptr); \
}

#define EMITH_JMP3_START(cond) { \
	u8 *cond_ptr, *else_ptr; \
	JMP8_POS(cond_ptr)

#define EMITH_JMP3_MID(cond) \
	JMP8_POS(else_ptr); \
	JMP8_EMIT(cond, cond_ptr);

#define EMITH_JMP3_END() \
	JMP8_EMIT_NC(else_ptr); \
}

// "simple" jump (no more then a few insns)
// ARM will use conditional instructions here
#define EMITH_SJMP_DECL_() \
	u8 *cond_ptr

#define EMITH_SJMP_START_(cond) \
	JMP8_POS(cond_ptr)

#define EMITH_SJMP_END_(cond) \
	JMP8_EMIT(cond, cond_ptr)

#define EMITH_SJMP_START EMITH_JMP_START
#define EMITH_SJMP_END EMITH_JMP_END

#define EMITH_SJMP3_START EMITH_JMP3_START
#define EMITH_SJMP3_MID EMITH_JMP3_MID
#define EMITH_SJMP3_END EMITH_JMP3_END

#define EMITH_SJMP2_START(cond) \
	EMITH_SJMP3_START(cond)
#define EMITH_SJMP2_MID(cond) \
	EMITH_SJMP3_MID(cond)
#define EMITH_SJMP2_END(cond) \
	EMITH_SJMP3_END()

#define emith_pass_arg_r(arg, reg) do { \
	int rd = 7; \
	host_arg2reg(rd, arg); \
	emith_move_r_r_ptr(rd, reg); \
} while (0)

#define emith_pass_arg_imm(arg, imm) do { \
	int rd = 7; \
	host_arg2reg(rd, arg); \
	emith_move_r_imm(rd, imm); \
} while (0)

#define host_instructions_updated(base, end)

#ifdef __x86_64__

#define PTR_SCALE 3
#define NA_TMP_REG xAX // non-arg tmp from reg_temp[]

#define EMIT_REX_IF(w, r, rm) do { \
	int r_ = (r) > 7 ? 1 : 0; \
	int rm_ = (rm) > 7 ? 1 : 0; \
	if ((w) | r_ | rm_) \
		EMIT_REX(1, r_, 0, rm_); \
} while (0)

#ifndef _WIN32

#define host_arg2reg(rd, arg) \
	switch (arg) { \
	case 0: rd = xDI; break; \
	case 1: rd = xSI; break; \
	case 2: rd = xDX; break; \
	default: rd = xCX; break; \
	}

#define emith_sh2_drc_entry() do { \
	emith_push(xBX); \
	emith_push(xBP); \
	emith_push(xSI); /* to align */ \
} while (0)

#define emith_sh2_drc_exit() do {  \
	emith_pop(xSI); \
	emith_pop(xBP); \
	emith_pop(xBX); \
	emith_ret(); \
} while (0)

#else // _WIN32

#define host_arg2reg(rd, arg) \
	switch (arg) { \
	case 0: rd = xCX; break; \
	case 1: rd = xDX; break; \
	case 2: rd = 8; break; \
	default: rd = 9; break; \
	}

#define emith_sh2_drc_entry() do { \
	emith_push(xBX); \
	emith_push(xBP); \
	emith_push(xSI); \
	emith_push(xDI); \
	emith_add_r_r_ptr_imm(xSP, xSP, -8*5); \
} while (0)

#define emith_sh2_drc_exit() do {  \
	emith_add_r_r_ptr_imm(xSP, xSP, 8*5); \
	emith_pop(xDI); \
	emith_pop(xSI); \
	emith_pop(xBP); \
	emith_pop(xBX); \
	emith_ret(); \
} while (0)

#endif // _WIN32

#else // !__x86_64__

#define PTR_SCALE 2
#define NA_TMP_REG xBX // non-arg tmp from reg_temp[]

#define EMIT_REX_IF(w, r, rm) do { \
	assert((u32)(r) < 8u); \
	assert((u32)(rm) < 8u); \
} while (0)

#define host_arg2reg(rd, arg) \
	switch (arg) { \
	case 0: rd = xAX; break; \
	case 1: rd = xDX; break; \
	case 2: rd = xCX; break; \
	default: rd = xBX; break; \
	}

#define emith_sh2_drc_entry() do { \
	emith_push(xBX);        \
	emith_push(xBP);        \
	emith_push(xSI);        \
	emith_push(xDI);        \
} while (0)

#define emith_sh2_drc_exit() do { \
	emith_pop(xDI);         \
	emith_pop(xSI);         \
	emith_pop(xBP);         \
	emith_pop(xBX);         \
	emith_ret();            \
} while (0)

#endif

#define emith_save_caller_regs(mask) do { \
	if ((mask) & (1 << xAX)) emith_push(xAX); \
	if ((mask) & (1 << xCX)) emith_push(xCX); \
	if ((mask) & (1 << xDX)) emith_push(xDX); \
	if ((mask) & (1 << xSI)) emith_push(xSI); \
	if ((mask) & (1 << xDI)) emith_push(xDI); \
} while (0)

#define emith_restore_caller_regs(mask) do { \
	if ((mask) & (1 << xDI)) emith_pop(xDI); \
	if ((mask) & (1 << xSI)) emith_pop(xSI); \
	if ((mask) & (1 << xDX)) emith_pop(xDX); \
	if ((mask) & (1 << xCX)) emith_pop(xCX); \
	if ((mask) & (1 << xAX)) emith_pop(xAX); \
} while (0)

#define emith_sh2_rcall(a, tab, func, mask) do { \
	emith_lsr(mask, a, SH2_READ_SHIFT); \
	EMIT_REX_IF(1, mask, tab); \
	EMIT_OP_MODRM64(0x8d, 0, tab, 4); \
	EMIT_SIB64(PTR_SCALE, mask, tab); /* lea tab, [tab + mask * {4,8}] */ \
	EMIT_REX_IF(1, mask, tab); \
	EMIT_OP_MODRM64(0x8d, 0, tab, 4); \
	EMIT_SIB64(PTR_SCALE, mask, tab); /* lea tab, [tab + mask * {4,8}] */ \
	EMIT_REX_IF(1, func, tab); \
	EMIT_OP_MODRM64(0x8b, 0, func, tab); /* mov func, [tab] */ \
	EMIT_OP_MODRM64(0x8b, 1, mask, tab); \
	EMIT(1 << PTR_SCALE, u8); /* mov mask, [tab + {4,8}] */ \
	emith_add_r_r_ptr(func, func); \
} while (0)

#define emith_sh2_wcall(a, val, tab, func) do { \
	int arg2_; \
	host_arg2reg(arg2_, 2); \
	emith_lsr(func, a, SH2_WRITE_SHIFT); /* tmp = a >> WRT_SHIFT */ \
	EMIT_REX_IF(1, func, tab); \
	EMIT_OP_MODRM64(0x8b, 0, func, 4); \
	EMIT_SIB64(PTR_SCALE, func, tab); /* mov tmp, [tab + tmp * {4,8}] */ \
	emith_move_r_r_ptr(arg2_, CONTEXT_REG); \
	emith_jump_reg(func); \
} while (0)

#define emith_sh2_dtbf_loop() do { \
	u8 *jmp0; /* negative cycles check */            \
	u8 *jmp1; /* unsinged overflow check */          \
	int cr, rn;                                      \
	int tmp_ = rcache_get_tmp();                     \
	cr = rcache_get_reg(SHR_SR, RC_GR_RMW);          \
	rn = rcache_get_reg((op >> 8) & 0x0f, RC_GR_RMW);\
	emith_sub_r_imm(rn, 1);                          \
	emith_sub_r_imm(cr, (cycles+1) << 12);           \
	cycles = 0;                                      \
	emith_asr(tmp_, cr, 2+12);                       \
	JMP8_POS(jmp0); /* no negative cycles */         \
	emith_move_r_imm(tmp_, 0);                       \
	JMP8_EMIT(ICOND_JNS, jmp0);                      \
	emith_and_r_imm(cr, 0xffe);                      \
	emith_subf_r_r(rn, tmp_);                        \
	JMP8_POS(jmp1); /* no overflow */                \
	emith_neg_r(rn); /* count left */                \
	emith_lsl(rn, rn, 2+12);                         \
	emith_or_r_r(cr, rn);                            \
	emith_or_r_imm(cr, 1);                           \
	emith_move_r_imm(rn, 0);                         \
	JMP8_EMIT(ICOND_JA, jmp1);                       \
	rcache_free_tmp(tmp_);                           \
} while (0)

#define emith_sh2_delay_loop(cycles, reg) do {			\
	int sr = rcache_get_reg(SHR_SR, RC_GR_RMW, NULL);	\
	int t1 = rcache_get_tmp();				\
	int t2 = rcache_get_tmp();				\
	int t3 = rcache_get_tmp();				\
	if (t3 == xAX) { t3 = t1; t1 = xAX; } /* for MUL */	\
	if (t3 == xDX) { t3 = t2; t2 = xDX; }			\
	/* if (sr < 0) return */				\
	emith_asrf(t2, sr, 12);					\
	EMITH_JMP_START(DCOND_LE);				\
	/* turns = sr.cycles / cycles */			\
	emith_move_r_imm(t3, (u32)((1ULL<<32) / (cycles)) + 1);	\
	emith_mul_u64(t1, t2, t2, t3); /* multiply by 1/x */	\
	rcache_free_tmp(t3);					\
	if (reg >= 0) {						\
		/* if (reg <= turns) turns = reg-1 */		\
		t3 = rcache_get_reg(reg, RC_GR_RMW, NULL);	\
		emith_cmp_r_r(t3, t2);				\
		EMITH_SJMP_START(DCOND_GT);			\
		emith_sub_r_r_imm_c(DCOND_LE, t2, t3, 1);	\
		EMITH_SJMP_END(DCOND_GT);			\
		/* if (reg <= 1) turns = 0 */			\
		emith_cmp_r_imm(t3, 1);				\
		EMITH_SJMP_START(DCOND_GT);			\
		emith_move_r_imm_c(DCOND_LE, t2, 0);		\
		EMITH_SJMP_END(DCOND_GT);			\
		/* reg -= turns */				\
		emith_sub_r_r(t3, t2);				\
	}							\
	/* sr.cycles -= turns * cycles; */			\
	emith_move_r_imm(t1, cycles);				\
	emith_mul_u64(t1, t2, t1, t2);				\
	emith_sub_r_r_r_lsl(sr, sr, t1, 12);			\
	EMITH_JMP_END(DCOND_LE);				\
	rcache_free_tmp(t1);					\
	rcache_free_tmp(t2);					\
} while (0)

#define emith_write_sr(sr, srcr) do { \
	int tmp_ = rcache_get_tmp(); \
	emith_clear_msb(tmp_, srcr, 22); \
	emith_bic_r_imm(sr, 0x3ff); \
	emith_or_r_r(sr, tmp_); \
	rcache_free_tmp(tmp_); \
} while (0)

#define emith_tpop_carry(sr, is_sub) \
	emith_lsr(sr, sr, 1)

#define emith_tpush_carry(sr, is_sub) \
	emith_adc_r_r(sr, sr)

/*
 * if Q
 *   t = carry(Rn += Rm)
 * else
 *   t = carry(Rn -= Rm)
 * T ^= t
 */
#define emith_sh2_div1_step(rn, rm, sr) do {      \
	u8 *jmp0, *jmp1;                          \
	int tmp_ = rcache_get_tmp();              \
	emith_eor_r_r(tmp_, tmp_);                \
	emith_tst_r_imm(sr, Q);  /* if (Q ^ M) */ \
	JMP8_POS(jmp0);          /* je do_sub */  \
	emith_add_r_r(rn, rm);                    \
	JMP8_POS(jmp1);          /* jmp done */   \
	JMP8_EMIT(ICOND_JE, jmp0); /* do_sub: */  \
	emith_sub_r_r(rn, rm);                    \
	JMP8_EMIT_NC(jmp1);      /* done: */      \
	emith_adc_r_r(tmp_, tmp_);                \
	emith_eor_r_r(sr, tmp_);                  \
	rcache_free_tmp(tmp_);                    \
} while (0)

/* mh:ml += rn*rm, does saturation if required by S bit. rn, rm must be TEMP */
#define emith_sh2_macl(ml, mh, rn, rm, sr) do {   \
	emith_tst_r_imm(sr, S);                   \
	EMITH_SJMP_START(DCOND_EQ);               \
	/* MACH top 16 bits unused if saturated. sign ext for overfl detect */ \
	emith_sext(mh, mh, 16);                   \
	EMITH_SJMP_END(DCOND_EQ);                 \
	emith_mula_s64(ml, mh, rn, rm);           \
	emith_tst_r_imm(sr, S);                   \
	EMITH_SJMP_START(DCOND_EQ);               \
	/* overflow if top 17 bits of MACH aren't all 1 or 0 */ \
	/* to check: add MACH[15] to MACH[31:16]. this is 0 if no overflow */ \
	emith_asrf(rn, mh, 16); /* sum = (MACH>>16) + ((MACH>>15)&1) */ \
	emith_adcf_r_imm(rn, 0); /* (MACH>>15) is in carry after shift */ \
	EMITH_SJMP_START(DCOND_EQ); /* sum != 0 -> ov */ \
	emith_move_r_imm_c(DCOND_NE, ml, 0x0000); /* -overflow */ \
	emith_move_r_imm_c(DCOND_NE, mh, 0x8000); \
	EMITH_SJMP_START(DCOND_LE); /* sum > 0 -> +ovl */ \
	emith_sub_r_imm_c(DCOND_GT, ml, 1); /* 0xffffffff */ \
	emith_sub_r_imm_c(DCOND_GT, mh, 1); /* 0x00007fff */ \
	EMITH_SJMP_END(DCOND_LE);                 \
	EMITH_SJMP_END(DCOND_EQ);                 \
	EMITH_SJMP_END(DCOND_EQ);                 \
} while (0)

/* mh:ml += rn*rm, does saturation if required by S bit. rn, rm must be TEMP */
#define emith_sh2_macw(ml, mh, rn, rm, sr) do {   \
	emith_tst_r_imm(sr, S);                   \
	EMITH_SJMP_START(DCOND_EQ);               \
	/* XXX: MACH should be untouched when S is set? */ \
	emith_asr(mh, ml, 31); /* sign ext MACL to MACH for ovrfl check */ \
	EMITH_SJMP_END(DCOND_EQ);                 \
	emith_mula_s64(ml, mh, rn, rm);           \
	emith_tst_r_imm(sr, S);                   \
	EMITH_SJMP_START(DCOND_EQ);               \
	/* overflow if top 33 bits of MACH:MACL aren't all 1 or 0 */ \
	/* to check: add MACL[31] to MACH. this is 0 if no overflow */ \
	emith_lsr(rn, ml, 31);                    \
	emith_addf_r_r(rn, mh); /* sum = MACH + ((MACL>>31)&1) */ \
	EMITH_SJMP_START(DCOND_EQ); /* sum != 0 -> overflow */ \
	/* XXX: LSB signalling only in SH1, or in SH2 too? */ \
	emith_move_r_imm_c(DCOND_NE, mh, 0x00000001); /* LSB of MACH */ \
	emith_move_r_imm_c(DCOND_NE, ml, 0x80000000); /* negative ovrfl */ \
	EMITH_SJMP_START(DCOND_LE); /* sum > 0 -> positive ovrfl */ \
	emith_sub_r_imm_c(DCOND_GT, ml, 1); /* 0x7fffffff */ \
	EMITH_SJMP_END(DCOND_LE);                 \
	EMITH_SJMP_END(DCOND_EQ);                 \
	EMITH_SJMP_END(DCOND_EQ);                 \
} while (0)

#define emith_pool_check()	/**/
#define emith_pool_commit(j)	/**/
#define emith_insn_ptr()	((u8 *)tcache_ptr)
#define	emith_flush()		/**/

#ifdef T
// T bit handling
#define emith_invert_cond(cond) \
	((cond) ^ 1)

static void emith_clr_t_cond(int sr)
{
  emith_bic_r_imm(sr, T);
}

static void emith_set_t_cond(int sr, int cond)
{
  EMITH_SJMP_START(emith_invert_cond(cond));
  emith_or_r_imm_c(cond, sr, T);
  EMITH_SJMP_END(emith_invert_cond(cond));
}

#define emith_get_t_cond()      -1

#define emith_sync_t(sr)	((void)sr)

#define emith_invalidate_t()

static void emith_set_t(int sr, int val)
{
  if (val) 
    emith_or_r_imm(sr, T);
  else
    emith_bic_r_imm(sr, T);
}

static int emith_tst_t(int sr, int tf)
{
  emith_tst_r_imm(sr, T);
  return tf ? DCOND_NE: DCOND_EQ;
}
#endif

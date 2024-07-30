#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/*
 * @namespace	usr_plugins_II
 * @file        plugins/disasm/ix86/ix86.c
 * @brief       This file contains implementation of Intel x86 disassembler (main module).
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
 * @author      Mauro Giachero
 * @date        11.2007
 * @note        Implemented x86 inline assembler as a NASM/YASM wrapper
**/
#include <iostream>
#include <sstream>
#include <fstream>

#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "beye.h"
#include "beyehelp.h"
#include "plugins/disasm.h"
#include "plugins/disasm/ix86/ix86.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "codeguid.h"
#include "listbox.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"
#include "libbeye/libbeye.h"
#include "libbeye/osdep/system.h"

namespace	usr {
const unsigned ix86_Disassembler::MAX_IX86_INSN_LEN=15;
const unsigned ix86_Disassembler::MAX_DISASM_OUTPUT=1000;

const char  ix86_Disassembler::ix86CloneSNames[4] = { 'i', 'a', 'c', 'v' };
const char* ix86_Disassembler::ix86_sizes[] = { "", "(b)", "(w)", "(d)", "(p)", "(q)", "(t)" };
const char* ix86_Disassembler::ix86_A16[] = { "bx+si", "bx+di", "bp+si", "bp+di", "si" , "di" , "bp" , "bx" };

const char* ix86_Disassembler::i8086_ByteRegs[] = { "al", "cl", "dl", "bl", "ah",  "ch",  "dh",  "bh" };
const char* ix86_Disassembler::k64_ByteRegs[]   = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };
const char* ix86_Disassembler::ix86_MMXRegs[]  = { "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7" };
const char* ix86_Disassembler::ix86_SegRegs[]  = { "es", "cs", "ss", "ds", "fs", "gs", "?s", "?s" };

const char* ix86_Disassembler::k64_WordRegs[] =  { "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
const char* ix86_Disassembler::k64_DWordRegs[] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
const char* ix86_Disassembler::k64_QWordRegs[] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8",  "r9",  "r10",  "r11",  "r12",  "r13",  "r14",  "r15" };
const char* ix86_Disassembler::k64_XMMXRegs[] = { "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15" };
const char* ix86_Disassembler::k64_YMMXRegs[] = { "ymm0","ymm1","ymm2","ymm3","ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11","ymm12","ymm13","ymm14","ymm15" };
const char* ix86_Disassembler::k64_CrxRegs[]  = { "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7", "cr8", "cr9", "cr10", "cr11", "cr12", "cr13", "cr14", "cr15" };
const char* ix86_Disassembler::k64_DrxRegs[]  = { "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7", "dr8", "dr9", "dr10", "dr11", "dr12", "dr13", "dr14", "dr15" };
const char* ix86_Disassembler::k64_TrxRegs[]  = { "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7", "tr8", "tr9", "tr10", "tr11", "tr12", "tr13", "tr14", "tr15" };
const char* ix86_Disassembler::k64_XrxRegs[]  = { "?r0", "?r1", "?r2", "?r3", "?r4", "?r5", "?r6", "?r7", "?r8", "?r9", "?r10", "?r11", "?r12", "?r13", "?r14", "?r15" };

#define DECLARE_BASE_INSN(n16, n32, n64, func, func64, flags, flags64) { n16, n32, n64, func, flags, func64, flags64 }

const ix86_Opcodes ix86_Disassembler::ix86_table[256] =
{
  /*0x00*/ DECLARE_BASE_INSN("add","add","add",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x01*/ DECLARE_BASE_INSN("add","add","add",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x02*/ DECLARE_BASE_INSN("add","add","add",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x03*/ DECLARE_BASE_INSN("add","add","add",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x04*/ DECLARE_BASE_INSN("add","add","add",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x05*/ DECLARE_BASE_INSN("add","add","add",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x06*/ DECLARE_BASE_INSN("push","push","???",&ix86_Disassembler::ix86_ArgES,&ix86_Disassembler::ix86_ArgES,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x07*/ DECLARE_BASE_INSN("pop","pop","???",&ix86_Disassembler::ix86_ArgES,&ix86_Disassembler::ix86_ArgES,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x08*/ DECLARE_BASE_INSN("or","or","or",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x09*/ DECLARE_BASE_INSN("or","or","or",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x0A*/ DECLARE_BASE_INSN("or","or","or",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x0B*/ DECLARE_BASE_INSN("or","or","or",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x0C*/ DECLARE_BASE_INSN("or","or","or",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x0D*/ DECLARE_BASE_INSN("or","or","or",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x0E*/ DECLARE_BASE_INSN("push","push","???", &ix86_Disassembler::ix86_ArgCS,&ix86_Disassembler::ix86_ArgCS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x0F*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ExOpCodes,&ix86_Disassembler::ix86_ExOpCodes,IX86_CPU086,K64_ATHLON),
  /*0x10*/ DECLARE_BASE_INSN("adc","adc","adc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x11*/ DECLARE_BASE_INSN("adc","adc","adc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x12*/ DECLARE_BASE_INSN("adc","adc","adc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x13*/ DECLARE_BASE_INSN("adc","adc","adc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x14*/ DECLARE_BASE_INSN("adc","adc","adc",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x15*/ DECLARE_BASE_INSN("adc","adc","adc",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x16*/ DECLARE_BASE_INSN("push","push","???",&ix86_Disassembler::ix86_ArgSS,&ix86_Disassembler::ix86_ArgSS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x17*/ DECLARE_BASE_INSN("pop","pop","???",&ix86_Disassembler::ix86_ArgSS,&ix86_Disassembler::ix86_ArgSS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x18*/ DECLARE_BASE_INSN("sbb","sbb","sbb",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x19*/ DECLARE_BASE_INSN("sbb","sbb","sbb",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x1A*/ DECLARE_BASE_INSN("sbb","sbb","sbb",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x1B*/ DECLARE_BASE_INSN("sbb","sbb","sbb",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x1C*/ DECLARE_BASE_INSN("sbb","sbb","sbb",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x1D*/ DECLARE_BASE_INSN("sbb","sbb","sbb",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x1E*/ DECLARE_BASE_INSN("push","push","???",&ix86_Disassembler::ix86_ArgDS,&ix86_Disassembler::ix86_ArgDS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x1F*/ DECLARE_BASE_INSN("pop","pop","???",&ix86_Disassembler::ix86_ArgDS,&ix86_Disassembler::ix86_ArgDS,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x20*/ DECLARE_BASE_INSN("and","and","and",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x21*/ DECLARE_BASE_INSN("and","and","and",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x22*/ DECLARE_BASE_INSN("and","and","and",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x23*/ DECLARE_BASE_INSN("and","and","and",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x24*/ DECLARE_BASE_INSN("and","and","and",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x25*/ DECLARE_BASE_INSN("and","and","and",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x26*/ DECLARE_BASE_INSN("seg","seg","seg",&ix86_Disassembler::ix86_ArgES,&ix86_Disassembler::ix86_ArgES,IX86_CPU086,K64_ATHLON),
  /*0x27*/ DECLARE_BASE_INSN("daa","daa","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x28*/ DECLARE_BASE_INSN("sub","sub","sub",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x29*/ DECLARE_BASE_INSN("sub","sub","sub",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x2A*/ DECLARE_BASE_INSN("sub","sub","sub",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x2B*/ DECLARE_BASE_INSN("sub","sub","sub",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x2C*/ DECLARE_BASE_INSN("sub","sub","sub",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x2D*/ DECLARE_BASE_INSN("sub","sub","sub",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x2E*/ DECLARE_BASE_INSN("seg","seg","seg",&ix86_Disassembler::ix86_ArgCS,&ix86_Disassembler::ix86_ArgCS,IX86_CPU086,K64_ATHLON),
  /*0x2F*/ DECLARE_BASE_INSN("das","das","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x30*/ DECLARE_BASE_INSN("xor","xor","xor",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x31*/ DECLARE_BASE_INSN("xor","xor","xor",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x32*/ DECLARE_BASE_INSN("xor","xor","xor",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x33*/ DECLARE_BASE_INSN("xor","xor","xor",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x34*/ DECLARE_BASE_INSN("xor","xor","xor",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x35*/ DECLARE_BASE_INSN("xor","xor","xor",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x36*/ DECLARE_BASE_INSN("seg","seg","seg",&ix86_Disassembler::ix86_ArgSS,&ix86_Disassembler::ix86_ArgSS,IX86_CPU086,K64_ATHLON),
  /*0x37*/ DECLARE_BASE_INSN("aaa","aaa","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x38*/ DECLARE_BASE_INSN("cmp","cmp","cmp",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x39*/ DECLARE_BASE_INSN("cmp","cmp","cmp",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x3A*/ DECLARE_BASE_INSN("cmp","cmp","cmp",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x3B*/ DECLARE_BASE_INSN("cmp","cmp","cmp",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x3C*/ DECLARE_BASE_INSN("cmp","cmp","cmp",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x3D*/ DECLARE_BASE_INSN("cmp","cmp","cmp",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0x3E*/ DECLARE_BASE_INSN("seg","seg","seg",&ix86_Disassembler::ix86_ArgDS,&ix86_Disassembler::ix86_ArgDS,IX86_CPU086,K64_ATHLON),
  /*0x3F*/ DECLARE_BASE_INSN("aas","aas","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x40*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x41*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x42*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x43*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x44*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x45*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x46*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x47*/ DECLARE_BASE_INSN("inc","inc","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x48*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x49*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x4A*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x4B*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x4C*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x4D*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x4E*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x4F*/ DECLARE_BASE_INSN("dec","dec","rex",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x50*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x51*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x52*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x53*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x54*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x55*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x56*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x57*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x58*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x59*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5A*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5B*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5C*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5D*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5E*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x5F*/ DECLARE_BASE_INSN("pop","pop","pop",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU086,K64_ATHLON|K64_FORCE64),
  /*0x60*/ DECLARE_BASE_INSN("pushaw","pushad","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186,K64_ATHLON|K64_NOCOMPAT),
  /*0x61*/ DECLARE_BASE_INSN("popaw","popad","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186,K64_ATHLON|K64_NOCOMPAT),
  /*0x62*/ DECLARE_BASE_INSN("bound","bound","???",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU286,K64_ATHLON|K64_NOCOMPAT),
  /*0x63*/ DECLARE_BASE_INSN("arpl","arpl","movsxd",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU286|INSN_STORE,K64_ATHLON|K64_NOCOMPAT),
  /*0x64*/ DECLARE_BASE_INSN("seg","seg","seg",&ix86_Disassembler::ix86_ArgFS,&ix86_Disassembler::ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0x65*/ DECLARE_BASE_INSN("seg","seg","seg",&ix86_Disassembler::ix86_ArgGS,&ix86_Disassembler::ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0x66*/ DECLARE_BASE_INSN("???","???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU386,K64_ATHLON),
  /*0x67*/ DECLARE_BASE_INSN("???","???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU386,K64_ATHLON),
  /*0x68*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_imm,&ix86_Disassembler::arg_imm,IX86_CPU186,K64_ATHLON),
  /*0x69*/ DECLARE_BASE_INSN("imul","imul","imul",&ix86_Disassembler::arg_cpu_modregrm_imm,&ix86_Disassembler::arg_cpu_modregrm_imm,IX86_CPU186,K64_ATHLON),
  /*0x6A*/ DECLARE_BASE_INSN("push","push","push",&ix86_Disassembler::arg_imm,&ix86_Disassembler::arg_imm,IX86_CPU186|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x6B*/ DECLARE_BASE_INSN("imul","imul","imul",&ix86_Disassembler::arg_cpu_modregrm_imm,&ix86_Disassembler::arg_cpu_modregrm_imm,IX86_CPU186|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x6C*/ DECLARE_BASE_INSN("insb","insb","insb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x6D*/ DECLARE_BASE_INSN("insw","insd","insd",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x6E*/ DECLARE_BASE_INSN("outsb","outsb","outsb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x6F*/ DECLARE_BASE_INSN("outsw","outsd","outsd",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x70*/ DECLARE_BASE_INSN("jo","jo","jo",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x71*/ DECLARE_BASE_INSN("jno","jno","jno",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x72*/ DECLARE_BASE_INSN("jc","jc","jc",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x73*/ DECLARE_BASE_INSN("jnc","jnc","jnc",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x74*/ DECLARE_BASE_INSN("je","je","je",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x75*/ DECLARE_BASE_INSN("jne","jne","jne",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x76*/ DECLARE_BASE_INSN("jna","jna","jna",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x77*/ DECLARE_BASE_INSN("ja","ja","ja",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x78*/ DECLARE_BASE_INSN("js","js","js",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x79*/ DECLARE_BASE_INSN("jns","jns","jns",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7A*/ DECLARE_BASE_INSN("jp","jp","jp",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7B*/ DECLARE_BASE_INSN("jnp","jnp","jnp",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7C*/ DECLARE_BASE_INSN("jl","jl","jl",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7D*/ DECLARE_BASE_INSN("jnl","jnl","jnl",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7E*/ DECLARE_BASE_INSN("jle","jle","jle",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x7F*/ DECLARE_BASE_INSN("jg","jg","jg",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0x80*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgOp1,&ix86_Disassembler::ix86_ArgOp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x81*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgOp1,&ix86_Disassembler::ix86_ArgOp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x82*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgOp2,&ix86_Disassembler::ix86_ArgOp2,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x83*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgOp2,&ix86_Disassembler::ix86_ArgOp2,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x84*/ DECLARE_BASE_INSN("test","test","test",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x85*/ DECLARE_BASE_INSN("test","test","test",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x86*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x87*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x88*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0x89*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x8A*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x8B*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x8C*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_modsegrm,&ix86_Disassembler::arg_cpu_modsegrm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x8D*/ DECLARE_BASE_INSN("lea","lea","lea",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU086,K64_ATHLON),
  /*0x8E*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_modsegrm,&ix86_Disassembler::arg_cpu_modsegrm,IX86_CPU086,K64_ATHLON),
  /*0x8F*/ DECLARE_BASE_INSN("pop","pop","xop",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x90*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x91*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x92*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x93*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x94*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x95*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x96*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x97*/ DECLARE_BASE_INSN("xchg","xchg","xchg",&ix86_Disassembler::arg_r0rm,&ix86_Disassembler::arg_r0rm,IX86_CPU086,K64_ATHLON),
  /*0x98*/ DECLARE_BASE_INSN("cbw","cwde","cdqe",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0x99*/ DECLARE_BASE_INSN("cwd","cdq","cqo",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0x9A*/ DECLARE_BASE_INSN("callf16","callf32","???",&ix86_Disassembler::arg_segoff,&ix86_Disassembler::arg_segoff,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x9B*/ DECLARE_BASE_INSN("wait","wait","wait",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0x9C*/ DECLARE_BASE_INSN("pushfw","pushfd","pushfq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0x9D*/ DECLARE_BASE_INSN("popfw","popfd","popfq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|K64_NOCOMPAT|INSN_CPL0),
  /*0x9E*/ DECLARE_BASE_INSN("sahf","sahf","sahf",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0x9F*/ DECLARE_BASE_INSN("lahf","lahf","lahf",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA0*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_r0mem,&ix86_Disassembler::arg_r0mem,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA1*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_r0mem,&ix86_Disassembler::arg_r0mem,IX86_CPU086,K64_ATHLON),
  /*0xA2*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_r0mem,&ix86_Disassembler::arg_r0mem,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xA3*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_r0mem,&ix86_Disassembler::arg_r0mem,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA4*/ DECLARE_BASE_INSN("movsb","movsb","movsb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA5*/ DECLARE_BASE_INSN("movsw","movsd","movsq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xA6*/ DECLARE_BASE_INSN("cmpsb","cmpsb","cmpsb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA7*/ DECLARE_BASE_INSN("cmpsw","cmpsd","cmpsq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xA8*/ DECLARE_BASE_INSN("test","test","test",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xA9*/ DECLARE_BASE_INSN("test","test","test",&ix86_Disassembler::arg_r0_imm,&ix86_Disassembler::arg_r0_imm,IX86_CPU086,K64_ATHLON),
  /*0xAA*/ DECLARE_BASE_INSN("stosb","stosb","stosb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xAB*/ DECLARE_BASE_INSN("stosw","stosd","stosq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xAC*/ DECLARE_BASE_INSN("lodsb","lodsb","lodsb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xAD*/ DECLARE_BASE_INSN("lodsw","lodsd","lodsq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xAE*/ DECLARE_BASE_INSN("scasb","scasb","scasb",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xAF*/ DECLARE_BASE_INSN("scasw","scasd","scasq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON|K64_DEF32),
  /*0xB0*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB1*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB2*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB3*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB4*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB5*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB6*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB7*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xB8*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xB9*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBA*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBB*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBC*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBD*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBE*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xBF*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_insnreg_imm,&ix86_Disassembler::arg_insnreg_imm,IX86_CPU086,K64_ATHLON),
  /*0xC0*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ShOp2,&ix86_Disassembler::ix86_ShOp2,IX86_CPU186|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xC1*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ShOp2,&ix86_Disassembler::ix86_ShOp2,IX86_CPU186|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC2*/ DECLARE_BASE_INSN("retn16","retn32","retn32",&ix86_Disassembler::arg_imm16,&ix86_Disassembler::arg_imm16,IX86_CPU086,K64_ATHLON),
  /*0xC3*/ DECLARE_BASE_INSN("retn16","retn32","retn32",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xC4*/ DECLARE_BASE_INSN("les","les","vex",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xC5*/ DECLARE_BASE_INSN("lds","lds","vex",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xC6*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_mod_rm_imm,&ix86_Disassembler::arg_cpu_mod_rm_imm,IX86_CPU086|INSN_OP_BYTE|IMM_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|IMM_BYTE|INSN_STORE),
  /*0xC7*/ DECLARE_BASE_INSN("mov","mov","mov",&ix86_Disassembler::arg_cpu_mod_rm_imm,&ix86_Disassembler::arg_cpu_mod_rm_imm,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC8*/ DECLARE_BASE_INSN("enter","enter","enter",&ix86_Disassembler::arg_imm16_imm8,&ix86_Disassembler::arg_imm16_imm8,IX86_CPU186,K64_ATHLON),
  /*0xC9*/ DECLARE_BASE_INSN("leave","leave","leave",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU186,K64_ATHLON),
  /*0xCA*/ DECLARE_BASE_INSN("retf16","retf32","retf32",&ix86_Disassembler::arg_imm16,&ix86_Disassembler::arg_imm16,IX86_CPU086,K64_ATHLON),
  /*0xCB*/ DECLARE_BASE_INSN("retf16","retf32","retf32",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xCC*/ DECLARE_BASE_INSN("int3","int3","int3",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xCD*/ DECLARE_BASE_INSN("int","int","int",&ix86_Disassembler::arg_imm8,&ix86_Disassembler::arg_imm8,IX86_CPU086,K64_ATHLON),
  /*0xCE*/ DECLARE_BASE_INSN("into16","into32","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xCF*/ DECLARE_BASE_INSN("iret16","iret32","iretq",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xD0*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ShOp1,&ix86_Disassembler::ix86_ShOp1,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xD1*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ShOp1,&ix86_Disassembler::ix86_ShOp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xD2*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ShOpCL,&ix86_Disassembler::ix86_ShOpCL,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xD3*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ShOpCL,&ix86_Disassembler::ix86_ShOpCL,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xD4*/ DECLARE_BASE_INSN("aam on","aam on","???",&ix86_Disassembler::arg_imm8,&ix86_Disassembler::arg_imm8,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0xD5*/ DECLARE_BASE_INSN("aad on","aad on","???",&ix86_Disassembler::arg_imm8,&ix86_Disassembler::arg_imm8,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0xD6*/ DECLARE_BASE_INSN("salc","salc","salc",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xD7*/ DECLARE_BASE_INSN("xlat","xlat","xlat",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xD8*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xD9*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDA*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDB*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDC*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDD*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDE*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xDF*/ DECLARE_BASE_INSN("esc","esc","esc",&ix86_Disassembler::ix86_FPUCmd,&ix86_Disassembler::ix86_FPUCmd,IX86_CPU086|INSN_FPU,K64_ATHLON|INSN_FPU),
  /*0xE0*/ DECLARE_BASE_INSN("loopnz","loopnz","loopnz",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE1*/ DECLARE_BASE_INSN("loopz","loopz","loopz",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE2*/ DECLARE_BASE_INSN("loop","loop","loop",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE3*/ DECLARE_BASE_INSN("jcxz","jecxz","jrcxz",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xE4*/ DECLARE_BASE_INSN("in","in","in",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xE5*/ DECLARE_BASE_INSN("in","in","in",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE6*/ DECLARE_BASE_INSN("out","out","out",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xE7*/ DECLARE_BASE_INSN("out","out","out",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xE8*/ DECLARE_BASE_INSN("calln16","calln32","calln32",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086,K64_ATHLON),
  /*0xE9*/ DECLARE_BASE_INSN("jmpn16","jmpn32","jmpn32",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086,K64_ATHLON),
  /*0xEA*/ DECLARE_BASE_INSN("jmpf16","jmpf32","???",&ix86_Disassembler::arg_segoff,&ix86_Disassembler::arg_segoff,IX86_CPU086,K64_ATHLON|K64_NOCOMPAT),
  /*0xEB*/ DECLARE_BASE_INSN("jmps","jmps","jmps",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU086|IMM_BYTE,K64_ATHLON|IMM_BYTE),
  /*0xEC*/ DECLARE_BASE_INSN("in","in","in",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xED*/ DECLARE_BASE_INSN("in","in","in",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xEE*/ DECLARE_BASE_INSN("out","out","out",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xEF*/ DECLARE_BASE_INSN("out","out","out",&ix86_Disassembler::ix86_InOut,&ix86_Disassembler::ix86_InOut,IX86_CPU086,K64_ATHLON),
  /*0xF0*/ DECLARE_BASE_INSN("lock","lock","lock",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xF1*/ DECLARE_BASE_INSN("icebp","icebp","icebp",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU386,K64_ATHLON),
  /*0xF2*/ DECLARE_BASE_INSN("repne","repne","repne",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xF3*/ DECLARE_BASE_INSN("rep","rep","rep",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xF4*/ DECLARE_BASE_INSN("hlt","hlt","hlt",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xF5*/ DECLARE_BASE_INSN("cmc","cmc","cmc",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xF6*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgGrp1,&ix86_Disassembler::ix86_ArgGrp1,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xF7*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgGrp1,&ix86_Disassembler::ix86_ArgGrp1,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xF8*/ DECLARE_BASE_INSN("clc","clc","clc",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xF9*/ DECLARE_BASE_INSN("stc","stc","stc",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xFA*/ DECLARE_BASE_INSN("cli","cli","cli",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xFB*/ DECLARE_BASE_INSN("sti","sti","sti",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xFC*/ DECLARE_BASE_INSN("cld","cld","cld",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xFD*/ DECLARE_BASE_INSN("std","std","std",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU086,K64_ATHLON),
  /*0xFE*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgGrp2,&ix86_Disassembler::ix86_ArgGrp2,IX86_CPU086|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xFF*/ DECLARE_BASE_INSN("!!!","!!!","!!!",&ix86_Disassembler::ix86_ArgGrp2,&ix86_Disassembler::ix86_ArgGrp2,IX86_CPU086|INSN_STORE,K64_ATHLON|INSN_STORE)
};

#define DECLARE_EX_INSN(n32, n64, func, func64, flags, flags64) { n32, n64, func, func64, flags64, flags }

const ix86_ExOpcodes ix86_Disassembler::ix86_0F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("pshufb", "pshufb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x01*/ DECLARE_EX_INSN("phaddw", "phaddw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x02*/ DECLARE_EX_INSN("phaddd", "phaddd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x03*/ DECLARE_EX_INSN("phaddsw", "phaddsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x04*/ DECLARE_EX_INSN("pmaddubsw", "pmaddubsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x05*/ DECLARE_EX_INSN("phsubw", "phsubw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x06*/ DECLARE_EX_INSN("phsubd", "phsubd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x07*/ DECLARE_EX_INSN("phsubsw", "phsubsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x08*/ DECLARE_EX_INSN("psingb", "psignb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x09*/ DECLARE_EX_INSN("psignw", "psignw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x0A*/ DECLARE_EX_INSN("psignd", "psignd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x0B*/ DECLARE_EX_INSN("pmulhrsw", "pmulhrsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("pabsb", "pabsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x1D*/ DECLARE_EX_INSN("pabsw", "pabsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x1E*/ DECLARE_EX_INSN("pabsd", "pabsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN("movbe", "movbe", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P8, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN("movbe", "movbe", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P8|INSN_STORE, K64_ATHLON|INSN_STORE),
  /*0xF2*/ DECLARE_EX_INSN("andn", "andn", &ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P9|INSN_VEX_V,K64_ATHLON|INSN_VEX_V),
  /*0xF3*/ DECLARE_EX_INSN("!!!", "!!!", &ix86_Disassembler::ix86_ArgBm1Grp,&ix86_Disassembler::ix86_ArgBm1Grp,IX86_P9|INSN_VEX_V,K64_ATHLON|INSN_VEX_V),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN("bzhi", "bzhi", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN("bextr", "bextr", &ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P9|INSN_VEX_V|INSN_VEXW_AS_SWAP,K64_ATHLON|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_0F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN("palignr", "palignr",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P6|INSN_MMX, K64_ATHLON|INSN_MMX),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_0FA6_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN("montmul", "montmul", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN("xsha1", "xsha1", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN("xsha256", "xsha256", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_0FA7_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN("xstore-rng", "xstore-rng", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN("xcryptecb", "xcryptecb", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN("xcryptcbc", "xcryptcbc", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN("xcryptctr", "xcryptctr", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN("xcryptcfb", "xcryptcfb", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN("xcryptofb", "xcryptofb", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_VIA, IX86_VIA),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

/*
  NOP:
  90
  66 90
  0F 1F /0
  0F 1F 00
  0F 1F 40 00
  0F 1F 44 00 00
  66 0F 1F 44 00 00
  0F 1F 80 00 00 00 00
  0F 1F 84 00 00 00 00 00
  66 0F 1F 84 00 00 00 00 00
*/

const ix86_ExOpcodes ix86_Disassembler::ix86_extable[256] = /* for 0FH leading */
{
  /*0x00*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgExGr0,&ix86_Disassembler::ix86_ArgExGr0,IX86_CPU286|INSN_STORE,K64_ATHLON|INSN_CPL0|INSN_STORE),
  /*0x01*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgExGr1,&ix86_Disassembler::ix86_ArgExGr1,IX86_CPU286|INSN_CPL0|INSN_STORE,K64_ATHLON|INSN_CPL0|INSN_STORE),
  /*0x02*/ DECLARE_EX_INSN("lar","lar",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU286|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x03*/ DECLARE_EX_INSN("lsl","lsl",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU286|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x04*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN("syscall","syscall",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_K6|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x06*/ DECLARE_EX_INSN("clts","clts",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU286|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x07*/ DECLARE_EX_INSN("sysret","sysret",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_K6|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x08*/ DECLARE_EX_INSN("invd","invd",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU486|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x09*/ DECLARE_EX_INSN("wbinvd","wbinvd",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU486|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x0A*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN("ud","ud",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU686,K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_3DNowPrefetchGrp,&ix86_Disassembler::ix86_3DNowPrefetchGrp,IX86_3DNOW,K64_ATHLON|INSN_MMX),
  /*0x0E*/ DECLARE_EX_INSN("femms","femms",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_3DNOW,K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_3DNowOpCodes,&ix86_Disassembler::ix86_3DNowOpCodes,IX86_3DNOW,K64_ATHLON|INSN_MMX),
  /*0x10*/ DECLARE_EX_INSN("movups","movups",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x11*/ DECLARE_EX_INSN("movups","movups",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movlps","movlps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x13*/ DECLARE_EX_INSN("movlps","movlps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x14*/ DECLARE_EX_INSN("unpcklps","unpcklps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x15*/ DECLARE_EX_INSN("unpckhps","unpckhps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x16*/ DECLARE_EX_INSN("movhps","movhps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x17*/ DECLARE_EX_INSN("movhps","movhps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x18*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgKatmaiGrp2,&ix86_Disassembler::ix86_ArgKatmaiGrp2,IX86_P3|INSN_SSE|INSN_OP_BYTE,K64_ATHLON|INSN_SSE|INSN_OP_BYTE),
  /*0x19*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN("nop","nop",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P5,K64_FAM9),
  /*0x20*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x21*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x22*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x23*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x24*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x25*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x26*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x27*/ DECLARE_EX_INSN("mov","mov",&ix86_Disassembler::ix86_ArgMovXRY,&ix86_Disassembler::ix86_ArgMovXRY,IX86_CPU386|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x28*/ DECLARE_EX_INSN("movaps","movaps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN("movaps","movaps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2A*/ DECLARE_EX_INSN("cvtpi2ps","cvtpi2ps",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntps","movntps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttps2pi","cvttps2pi",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtps2pi","cvtps2pi",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2E*/ DECLARE_EX_INSN("ucomiss","ucomiss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN("comiss","comiss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN("wrmsr","wrmsr",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x31*/ DECLARE_EX_INSN("rdtsc","rdtsc",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x32*/ DECLARE_EX_INSN("rdmsr","rdmsr",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x33*/ DECLARE_EX_INSN("rdpmc","rdpmc",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU686|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0x34*/ DECLARE_EX_INSN("sysenter","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_P2|INSN_CPL0,K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN("sysexit","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_P2|INSN_CPL0,K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN("rdshr","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CYRIX686,K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN("getsec","getsec",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_P6,K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN((const char*)ix86_0F38_Table,(const char*)ix86_0F38_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN("dmint","dmint",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN((const char*)ix86_0F3A_Table,(const char*)ix86_0F3A_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN("cmovo","cmovo",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN("cmovno","cmovno",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN("cmovc","cmovc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN("cmovnc","cmovnc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN("cmove","cmove",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN("cmovne","cmovne",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN("cmovna","cmovna",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN("cmova","cmova",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN("cmovs","cmovs",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN("cmovns","cmovns",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN("cmovp","cmovp",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN("cmovnp","cmovnp",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN("cmovl","cmovl",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN("cmovnl","cmovnl",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN("cmovle","cmovle",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN("cmovg","cmovg",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU686,K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN("movmskps","movmskps",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_STORE|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_STORE|BRIDGE_CPU_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtps","sqrtps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x52*/ DECLARE_EX_INSN("rsqrtps","rsqrtps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x53*/ DECLARE_EX_INSN("rcpps","rcpps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x54*/ DECLARE_EX_INSN("andps","andps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x55*/ DECLARE_EX_INSN("andnps","andnps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x56*/ DECLARE_EX_INSN("orps","orps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x57*/ DECLARE_EX_INSN("xorps","xorps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x58*/ DECLARE_EX_INSN("addps","addps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulps","mulps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtps2pd","cvtps2pd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN("cvtdq2ps","cvtdq2ps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5C*/ DECLARE_EX_INSN("subps","subps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minps","minps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divps","divps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxps","maxps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN("punpcklbw","punpcklbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x61*/ DECLARE_EX_INSN("punpcklwd","punpcklwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x62*/ DECLARE_EX_INSN("punpckldq","punpckldq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x63*/ DECLARE_EX_INSN("packsswb","packsswb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x64*/ DECLARE_EX_INSN("pcmpgtb","pcmpgtb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x65*/ DECLARE_EX_INSN("pcmpgtw","pcmpgtw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x66*/ DECLARE_EX_INSN("pcmpgtd","pcmpgtd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x67*/ DECLARE_EX_INSN("packuswb","packuswb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x68*/ DECLARE_EX_INSN("punpckhbw","punpckhbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x69*/ DECLARE_EX_INSN("punpckhwd","punpckhwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6A*/ DECLARE_EX_INSN("punpckhdq","punpckhdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6B*/ DECLARE_EX_INSN("packssdw","packssdw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6C*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN("movd","movd",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x6F*/ DECLARE_EX_INSN("movq","movq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x70*/ DECLARE_EX_INSN("pshufw","pshufw",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x71*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgMMXGr1,&ix86_Disassembler::ix86_ArgMMXGr1,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x72*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgMMXGr2,&ix86_Disassembler::ix86_ArgMMXGr2,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x73*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgMMXGr3,&ix86_Disassembler::ix86_ArgMMXGr3,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x74*/ DECLARE_EX_INSN("pcmpeqb","pcmpeqb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x75*/ DECLARE_EX_INSN("pcmpeqw","pcmpeqw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x76*/ DECLARE_EX_INSN("pcmpeqd","pcmpeqd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x77*/ DECLARE_EX_INSN("emms","emms",&ix86_Disassembler::arg_emms,&ix86_Disassembler::arg_emms,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0x78*/ DECLARE_EX_INSN("vmread","vmread",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P6|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0x79*/ DECLARE_EX_INSN("vmwrite","vmwrite",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P6,K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN("svldt","???",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN("rsldt","???",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN("svts","???",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN("rsts","???",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CYRIX486,K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN("movd","movd",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_CPU586|INSN_MMX|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_STORE),
  /*0x7F*/ DECLARE_EX_INSN("movq","movq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_STORE),
  /*0x80*/ DECLARE_EX_INSN("jo","jo",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN("jno","jno",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN("jc","jc",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN("jnc","jnc",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN("je","je",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN("jne","jne",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN("jna","jna",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN("ja","ja",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN("js","js",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN("jns","jns",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN("jp","jp",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN("jnp","jnp",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN("jl","jl",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN("jnl","jnl",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN("jle","jle",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN("jg","jg",&ix86_Disassembler::arg_offset,&ix86_Disassembler::arg_offset,IX86_CPU386,K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN("seto","seto",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x91*/ DECLARE_EX_INSN("setno","setno",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x92*/ DECLARE_EX_INSN("setc","setc",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x93*/ DECLARE_EX_INSN("setnc","setnc",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x94*/ DECLARE_EX_INSN("sete","sete",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x95*/ DECLARE_EX_INSN("setne","setne",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x96*/ DECLARE_EX_INSN("setna","setna",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x97*/ DECLARE_EX_INSN("seta","seta",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x98*/ DECLARE_EX_INSN("sets","sets",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x99*/ DECLARE_EX_INSN("setns","setns",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9A*/ DECLARE_EX_INSN("setp","setp",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9B*/ DECLARE_EX_INSN("setnp","setnp",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9C*/ DECLARE_EX_INSN("setl","setl",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9D*/ DECLARE_EX_INSN("setnl","setnl",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9E*/ DECLARE_EX_INSN("setle","setle",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0x9F*/ DECLARE_EX_INSN("setg","setg",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_STORE|INSN_OP_BYTE),
  /*0xA0*/ DECLARE_EX_INSN("push","push",&ix86_Disassembler::ix86_ArgFS,&ix86_Disassembler::ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN("pop","pop",&ix86_Disassembler::ix86_ArgFS,&ix86_Disassembler::ix86_ArgFS,IX86_CPU386,K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN("cpuid","cpuid",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU486,K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN("bt","bt",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA4*/ DECLARE_EX_INSN("shld","shld",&ix86_Disassembler::ix86_DblShift,&ix86_Disassembler::ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA5*/ DECLARE_EX_INSN("shld","shld",&ix86_Disassembler::ix86_DblShift,&ix86_Disassembler::ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xA6*/ DECLARE_EX_INSN((const char*)ix86_0FA6_Table,(const char*)ix86_0FA6_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0xA7*/ DECLARE_EX_INSN((const char*)ix86_0FA7_Table,(const char*)ix86_0FA7_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0xA8*/ DECLARE_EX_INSN("push","push",&ix86_Disassembler::ix86_ArgGS,&ix86_Disassembler::ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN("pop","pop",&ix86_Disassembler::ix86_ArgGS,&ix86_Disassembler::ix86_ArgGS,IX86_CPU386,K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN("rsm","rsm",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU586|INSN_CPL0,K64_ATHLON|INSN_CPL0),
  /*0xAB*/ DECLARE_EX_INSN("bts","bts",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xAC*/ DECLARE_EX_INSN("shrd","shrd",&ix86_Disassembler::ix86_DblShift,&ix86_Disassembler::ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xAD*/ DECLARE_EX_INSN("shrd","shrd",&ix86_Disassembler::ix86_DblShift,&ix86_Disassembler::ix86_DblShift,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xAE*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgKatmaiGrp1,&ix86_Disassembler::ix86_ArgKatmaiGrp1,IX86_P3|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0xAF*/ DECLARE_EX_INSN("imul","imul",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN("cmpxchg","cmpxchg",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU486|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xB1*/ DECLARE_EX_INSN("cmpxchg","cmpxchg",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU486|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xB2*/ DECLARE_EX_INSN("lss","lss",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN("btr","btr",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xB4*/ DECLARE_EX_INSN("lfs","lfs",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN("lgs","lgs",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN("movzx","movzx",&ix86_Disassembler::ix86_ArgMovYX,&ix86_Disassembler::ix86_ArgMovYX,IX86_CPU386|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xB7*/ DECLARE_EX_INSN("movzx","movzx",&ix86_Disassembler::ix86_ArgMovYX,&ix86_Disassembler::ix86_ArgMovYX,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xB8*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKCPU,K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN("ud2","ud2",&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_CPU686,K64_ATHLON|INSN_CPL0),
  /*0xBA*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_BitGrp,&ix86_Disassembler::ix86_BitGrp,IX86_CPU386,K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN("btc","btc",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xBC*/ DECLARE_EX_INSN("bsf","bsf",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN("bsr","bsr",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU386,K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN("movsx","movsx",&ix86_Disassembler::ix86_ArgMovYX,&ix86_Disassembler::ix86_ArgMovYX,IX86_CPU386|INSN_OP_BYTE,K64_ATHLON|INSN_OP_BYTE),
  /*0xBF*/ DECLARE_EX_INSN("movsx","movsx",&ix86_Disassembler::ix86_ArgMovYX,&ix86_Disassembler::ix86_ArgMovYX,IX86_CPU386,K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN("xadd","xadd",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU486|INSN_OP_BYTE|INSN_STORE,K64_ATHLON|INSN_OP_BYTE|INSN_STORE),
  /*0xC1*/ DECLARE_EX_INSN("xadd","xadd",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_CPU486|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC2*/ DECLARE_EX_INSN("cmpps","cmpps",&ix86_Disassembler::arg_simd_cmp,&ix86_Disassembler::arg_simd_cmp,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN("movnti","movnti",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P4|INSN_STORE,K64_ATHLON|INSN_STORE),
  /*0xC4*/ DECLARE_EX_INSN("pinsrw","pinsrw",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8,IX86_P3|INSN_MMX|INSN_VEX_V,K64_ATHLON|INSN_MMX|INSN_VEX_V),
  /*0xC5*/ DECLARE_EX_INSN("pextrw","pextrw",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8,IX86_P3|INSN_MMX|INSN_VEX_V|BRIDGE_CPU_SSE|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_VEX_V|BRIDGE_CPU_SSE|INSN_STORE),
  /*0xC6*/ DECLARE_EX_INSN("shufps","shufps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC7*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_0FVMX,&ix86_Disassembler::ix86_0FVMX,IX86_CPU586,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN("bswap","bswap",&ix86_Disassembler::arg_insnreg,&ix86_Disassembler::arg_insnreg,IX86_CPU486,K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN("psrlw","psrlw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD2*/ DECLARE_EX_INSN("psrld","psrld",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD3*/ DECLARE_EX_INSN("psrlq","psrlq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD4*/ DECLARE_EX_INSN("paddq","paddq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD5*/ DECLARE_EX_INSN("pmullw","pmullw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD6*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN("pmovmskb","pmovmskb",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P3|INSN_MMX|BRIDGE_CPU_SSE,K64_ATHLON|INSN_MMX|BRIDGE_CPU_SSE),
  /*0xD8*/ DECLARE_EX_INSN("psubusb","psubusb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xD9*/ DECLARE_EX_INSN("psubusw","psubusw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDA*/ DECLARE_EX_INSN("pminub","pminub",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDB*/ DECLARE_EX_INSN("pand","pand",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDC*/ DECLARE_EX_INSN("paddusb","paddusb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDD*/ DECLARE_EX_INSN("paddusw","paddusw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDE*/ DECLARE_EX_INSN("pmaxub","pmaxub",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xDF*/ DECLARE_EX_INSN("pandn","pandn",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE0*/ DECLARE_EX_INSN("pavgb","pavgb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE1*/ DECLARE_EX_INSN("psraw","psraw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE2*/ DECLARE_EX_INSN("psrad","psrad",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE3*/ DECLARE_EX_INSN("pavgw","pavgw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE4*/ DECLARE_EX_INSN("pmulhuw","pmulhuw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE5*/ DECLARE_EX_INSN("pmulhw","pmulhw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE6*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN("movntq","movntq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX|INSN_STORE,K64_ATHLON|INSN_MMX|INSN_STORE),
  /*0xE8*/ DECLARE_EX_INSN("psubsb","psubsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xE9*/ DECLARE_EX_INSN("psubsw","psubsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEA*/ DECLARE_EX_INSN("pminsw","pminsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEB*/ DECLARE_EX_INSN("por","por",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEC*/ DECLARE_EX_INSN("paddsb","paddsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xED*/ DECLARE_EX_INSN("paddsw","paddsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEE*/ DECLARE_EX_INSN("pmaxsw","pmaxsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xEF*/ DECLARE_EX_INSN("pxor","pxor",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF0*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_UNKMMX,K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN("psllw","psllw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF2*/ DECLARE_EX_INSN("pslld","pslld",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF3*/ DECLARE_EX_INSN("psllq","psllq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF4*/ DECLARE_EX_INSN("pmuludq","pmuludq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF5*/ DECLARE_EX_INSN("pmaddwd","pmaddwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF6*/ DECLARE_EX_INSN("psadbw","psadbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF7*/ DECLARE_EX_INSN("maskmovq","maskmovq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF8*/ DECLARE_EX_INSN("psubb","psubb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xF9*/ DECLARE_EX_INSN("psubw","psubw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFA*/ DECLARE_EX_INSN("psubd","psubd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFB*/ DECLARE_EX_INSN("psubq","psubq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFC*/ DECLARE_EX_INSN("paddb","paddb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFD*/ DECLARE_EX_INSN("paddw","paddw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFE*/ DECLARE_EX_INSN("paddd","paddd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_CPU586|INSN_MMX,K64_ATHLON|INSN_MMX),
  /*0xFF*/ DECLARE_EX_INSN("???","???",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_UNKMMX,K64_ATHLON)
};

const ix86_3dNowopcodes ix86_Disassembler::ix86_3DNowtable[256] =
{
  /*0x00*/ { "???", IX86_UNKAMD },
  /*0x01*/ { "???", IX86_UNKAMD },
  /*0x02*/ { "???", IX86_UNKAMD },
  /*0x03*/ { "???", IX86_UNKAMD },
  /*0x04*/ { "???", IX86_UNKAMD },
  /*0x05*/ { "???", IX86_UNKAMD },
  /*0x06*/ { "???", IX86_UNKAMD },
  /*0x07*/ { "???", IX86_UNKAMD },
  /*0x08*/ { "???", IX86_UNKAMD },
  /*0x09*/ { "???", IX86_UNKAMD },
  /*0x0A*/ { "???", IX86_UNKAMD },
  /*0x0B*/ { "???", IX86_UNKAMD },
  /*0x0C*/ { "pi2fw", IX86_ATHLON },
  /*0x0D*/ { "pi2fd", IX86_3DNOW },
  /*0x0E*/ { "???", IX86_UNKAMD },
  /*0x0F*/ { "???", IX86_UNKAMD },
  /*0x10*/ { "???", IX86_UNKAMD },
  /*0x11*/ { "???", IX86_UNKAMD },
  /*0x12*/ { "???", IX86_UNKAMD },
  /*0x13*/ { "???", IX86_UNKAMD },
  /*0x14*/ { "???", IX86_UNKAMD },
  /*0x15*/ { "???", IX86_UNKAMD },
  /*0x16*/ { "???", IX86_UNKAMD },
  /*0x17*/ { "???", IX86_UNKAMD },
  /*0x18*/ { "???", IX86_UNKAMD },
  /*0x19*/ { "???", IX86_UNKAMD },
  /*0x1A*/ { "???", IX86_UNKAMD },
  /*0x1B*/ { "???", IX86_UNKAMD },
  /*0x1C*/ { "pf2iw", IX86_ATHLON },
  /*0x1D*/ { "pf2id", IX86_3DNOW },
  /*0x1E*/ { "???", IX86_UNKAMD },
  /*0x1F*/ { "???", IX86_UNKAMD },
  /*0x20*/ { "???", IX86_UNKAMD },
  /*0x21*/ { "???", IX86_UNKAMD },
  /*0x22*/ { "???", IX86_UNKAMD },
  /*0x23*/ { "???", IX86_UNKAMD },
  /*0x24*/ { "???", IX86_UNKAMD },
  /*0x25*/ { "???", IX86_UNKAMD },
  /*0x26*/ { "???", IX86_UNKAMD },
  /*0x27*/ { "???", IX86_UNKAMD },
  /*0x28*/ { "???", IX86_UNKAMD },
  /*0x29*/ { "???", IX86_UNKAMD },
  /*0x2A*/ { "???", IX86_UNKAMD },
  /*0x2B*/ { "???", IX86_UNKAMD },
  /*0x2C*/ { "???", IX86_UNKAMD },
  /*0x2D*/ { "???", IX86_UNKAMD },
  /*0x2E*/ { "???", IX86_UNKAMD },
  /*0x2F*/ { "???", IX86_UNKAMD },
  /*0x30*/ { "???", IX86_UNKAMD },
  /*0x31*/ { "???", IX86_UNKAMD },
  /*0x32*/ { "???", IX86_UNKAMD },
  /*0x33*/ { "???", IX86_UNKAMD },
  /*0x34*/ { "???", IX86_UNKAMD },
  /*0x35*/ { "???", IX86_UNKAMD },
  /*0x36*/ { "???", IX86_UNKAMD },
  /*0x37*/ { "???", IX86_UNKAMD },
  /*0x38*/ { "???", IX86_UNKAMD },
  /*0x39*/ { "???", IX86_UNKAMD },
  /*0x3A*/ { "???", IX86_UNKAMD },
  /*0x3B*/ { "???", IX86_UNKAMD },
  /*0x3C*/ { "???", IX86_UNKAMD },
  /*0x3D*/ { "???", IX86_UNKAMD },
  /*0x3E*/ { "???", IX86_UNKAMD },
  /*0x3F*/ { "???", IX86_UNKAMD },
  /*0x40*/ { "???", IX86_UNKAMD },
  /*0x41*/ { "???", IX86_UNKAMD },
  /*0x42*/ { "???", IX86_UNKAMD },
  /*0x43*/ { "???", IX86_UNKAMD },
  /*0x44*/ { "???", IX86_UNKAMD },
  /*0x45*/ { "???", IX86_UNKAMD },
  /*0x46*/ { "???", IX86_UNKAMD },
  /*0x47*/ { "???", IX86_UNKAMD },
  /*0x48*/ { "???", IX86_UNKAMD },
  /*0x49*/ { "???", IX86_UNKAMD },
  /*0x4A*/ { "???", IX86_UNKAMD },
  /*0x4B*/ { "???", IX86_UNKAMD },
  /*0x4C*/ { "???", IX86_UNKAMD },
  /*0x4D*/ { "???", IX86_UNKAMD },
  /*0x4E*/ { "???", IX86_UNKAMD },
  /*0x4F*/ { "???", IX86_UNKAMD },
  /*0x50*/ { "???", IX86_UNKAMD },
  /*0x51*/ { "???", IX86_UNKAMD },
  /*0x52*/ { "???", IX86_UNKAMD },
  /*0x53*/ { "???", IX86_UNKAMD },
  /*0x54*/ { "???", IX86_UNKAMD },
  /*0x55*/ { "???", IX86_UNKAMD },
  /*0x56*/ { "???", IX86_UNKAMD },
  /*0x57*/ { "???", IX86_UNKAMD },
  /*0x58*/ { "???", IX86_UNKAMD },
  /*0x59*/ { "???", IX86_UNKAMD },
  /*0x5A*/ { "???", IX86_UNKAMD },
  /*0x5B*/ { "???", IX86_UNKAMD },
  /*0x5C*/ { "???", IX86_UNKAMD },
  /*0x5D*/ { "???", IX86_UNKAMD },
  /*0x5E*/ { "???", IX86_UNKAMD },
  /*0x5F*/ { "???", IX86_UNKAMD },
  /*0x60*/ { "???", IX86_UNKAMD },
  /*0x61*/ { "???", IX86_UNKAMD },
  /*0x62*/ { "???", IX86_UNKAMD },
  /*0x63*/ { "???", IX86_UNKAMD },
  /*0x64*/ { "???", IX86_UNKAMD },
  /*0x65*/ { "???", IX86_UNKAMD },
  /*0x66*/ { "???", IX86_UNKAMD },
  /*0x67*/ { "???", IX86_UNKAMD },
  /*0x68*/ { "???", IX86_UNKAMD },
  /*0x69*/ { "???", IX86_UNKAMD },
  /*0x6A*/ { "???", IX86_UNKAMD },
  /*0x6B*/ { "???", IX86_UNKAMD },
  /*0x6C*/ { "???", IX86_UNKAMD },
  /*0x6D*/ { "???", IX86_UNKAMD },
  /*0x6E*/ { "???", IX86_UNKAMD },
  /*0x6F*/ { "???", IX86_UNKAMD },
  /*0x70*/ { "???", IX86_UNKAMD },
  /*0x71*/ { "???", IX86_UNKAMD },
  /*0x72*/ { "???", IX86_UNKAMD },
  /*0x73*/ { "???", IX86_UNKAMD },
  /*0x74*/ { "???", IX86_UNKAMD },
  /*0x75*/ { "???", IX86_UNKAMD },
  /*0x76*/ { "???", IX86_UNKAMD },
  /*0x77*/ { "???", IX86_UNKAMD },
  /*0x78*/ { "???", IX86_UNKAMD },
  /*0x79*/ { "???", IX86_UNKAMD },
  /*0x7A*/ { "???", IX86_UNKAMD },
  /*0x7B*/ { "???", IX86_UNKAMD },
  /*0x7C*/ { "???", IX86_UNKAMD },
  /*0x7D*/ { "???", IX86_UNKAMD },
  /*0x7E*/ { "???", IX86_UNKAMD },
  /*0x7F*/ { "???", IX86_UNKAMD },
  /*0x80*/ { "???", IX86_UNKAMD },
  /*0x81*/ { "???", IX86_UNKAMD },
  /*0x82*/ { "???", IX86_UNKAMD },
  /*0x83*/ { "???", IX86_UNKAMD },
  /*0x84*/ { "???", IX86_UNKAMD },
  /*0x85*/ { "???", IX86_UNKAMD },
  /*0x86*/ { "pfrcpv", IX86_GEODE },
  /*0x87*/ { "pfrsqrtv", IX86_GEODE },
  /*0x88*/ { "???", IX86_UNKAMD },
  /*0x89*/ { "???", IX86_UNKAMD },
  /*0x8A*/ { "pfnacc", IX86_ATHLON },
  /*0x8B*/ { "???", IX86_UNKAMD },
  /*0x8C*/ { "???", IX86_UNKAMD },
  /*0x8D*/ { "???", IX86_UNKAMD },
  /*0x8E*/ { "pfpnacc", IX86_ATHLON },
  /*0x8F*/ { "???", IX86_UNKAMD },
  /*0x90*/ { "pfcmpge", IX86_3DNOW },
  /*0x91*/ { "???", IX86_UNKAMD },
  /*0x92*/ { "???", IX86_UNKAMD },
  /*0x93*/ { "???", IX86_UNKAMD },
  /*0x94*/ { "pfmin", IX86_3DNOW },
  /*0x95*/ { "???", IX86_UNKAMD },
  /*0x96*/ { "pfrcp", IX86_3DNOW },
  /*0x97*/ { "pfrsqrt", IX86_3DNOW },
  /*0x98*/ { "???", IX86_UNKAMD },
  /*0x99*/ { "???", IX86_UNKAMD },
  /*0x9A*/ { "pfsub", IX86_3DNOW },
  /*0x9B*/ { "???", IX86_UNKAMD },
  /*0x9C*/ { "???", IX86_UNKAMD },
  /*0x9D*/ { "???", IX86_UNKAMD },
  /*0x9E*/ { "pfadd", IX86_3DNOW },
  /*0x9F*/ { "???", IX86_UNKAMD },
  /*0xA0*/ { "pfcmpgt", IX86_3DNOW },
  /*0xA1*/ { "???", IX86_UNKAMD },
  /*0xA2*/ { "???", IX86_UNKAMD },
  /*0xA3*/ { "???", IX86_UNKAMD },
  /*0xA4*/ { "pfmax", IX86_3DNOW },
  /*0xA5*/ { "???", IX86_UNKAMD },
  /*0xA6*/ { "pfrcpit1", IX86_3DNOW },
  /*0xA7*/ { "pfrsqit1", IX86_3DNOW },
  /*0xA8*/ { "???", IX86_UNKAMD },
  /*0xA9*/ { "???", IX86_UNKAMD },
  /*0xAA*/ { "pfsubr", IX86_3DNOW },
  /*0xAB*/ { "???", IX86_UNKAMD },
  /*0xAC*/ { "???", IX86_UNKAMD },
  /*0xAD*/ { "???", IX86_UNKAMD },
  /*0xAE*/ { "pfacc", IX86_3DNOW },
  /*0xAF*/ { "???", IX86_UNKAMD },
  /*0xB0*/ { "pfcmpeq", IX86_3DNOW },
  /*0xB1*/ { "???", IX86_UNKAMD },
  /*0xB2*/ { "???", IX86_UNKAMD },
  /*0xB3*/ { "???", IX86_UNKAMD },
  /*0xB4*/ { "pfmul", IX86_3DNOW },
  /*0xB5*/ { "???", IX86_UNKAMD },
  /*0xB6*/ { "pfrcpit2", IX86_3DNOW },
  /*0xB7*/ { "pmulhrwa", IX86_3DNOW },
  /*0xB8*/ { "???", IX86_UNKAMD },
  /*0xB9*/ { "???", IX86_UNKAMD },
  /*0xBA*/ { "???", IX86_UNKAMD },
  /*0xBB*/ { "pswapd", IX86_ATHLON },
  /*0xBC*/ { "???", IX86_UNKAMD },
  /*0xBD*/ { "???", IX86_UNKAMD },
  /*0xBE*/ { "???", IX86_UNKAMD },
  /*0xBF*/ { "pavgusb", IX86_3DNOW },
  /*0xC0*/ { "???", IX86_UNKAMD },
  /*0xC1*/ { "???", IX86_UNKAMD },
  /*0xC2*/ { "???", IX86_UNKAMD },
  /*0xC3*/ { "???", IX86_UNKAMD },
  /*0xC4*/ { "???", IX86_UNKAMD },
  /*0xC5*/ { "???", IX86_UNKAMD },
  /*0xC6*/ { "???", IX86_UNKAMD },
  /*0xC7*/ { "???", IX86_UNKAMD },
  /*0xC8*/ { "???", IX86_UNKAMD },
  /*0xC9*/ { "???", IX86_UNKAMD },
  /*0xCA*/ { "???", IX86_UNKAMD },
  /*0xCB*/ { "???", IX86_UNKAMD },
  /*0xCC*/ { "???", IX86_UNKAMD },
  /*0xCD*/ { "???", IX86_UNKAMD },
  /*0xCE*/ { "???", IX86_UNKAMD },
  /*0xCF*/ { "???", IX86_UNKAMD },
  /*0xD0*/ { "???", IX86_UNKAMD },
  /*0xD1*/ { "???", IX86_UNKAMD },
  /*0xD2*/ { "???", IX86_UNKAMD },
  /*0xD3*/ { "???", IX86_UNKAMD },
  /*0xD4*/ { "???", IX86_UNKAMD },
  /*0xD5*/ { "???", IX86_UNKAMD },
  /*0xD6*/ { "???", IX86_UNKAMD },
  /*0xD7*/ { "???", IX86_UNKAMD },
  /*0xD8*/ { "???", IX86_UNKAMD },
  /*0xD9*/ { "???", IX86_UNKAMD },
  /*0xDA*/ { "???", IX86_UNKAMD },
  /*0xDB*/ { "???", IX86_UNKAMD },
  /*0xDC*/ { "???", IX86_UNKAMD },
  /*0xDD*/ { "???", IX86_UNKAMD },
  /*0xDE*/ { "???", IX86_UNKAMD },
  /*0xDF*/ { "???", IX86_UNKAMD },
  /*0xE0*/ { "???", IX86_UNKAMD },
  /*0xE1*/ { "???", IX86_UNKAMD },
  /*0xE2*/ { "???", IX86_UNKAMD },
  /*0xE3*/ { "???", IX86_UNKAMD },
  /*0xE4*/ { "???", IX86_UNKAMD },
  /*0xE5*/ { "???", IX86_UNKAMD },
  /*0xE6*/ { "???", IX86_UNKAMD },
  /*0xE7*/ { "???", IX86_UNKAMD },
  /*0xE8*/ { "???", IX86_UNKAMD },
  /*0xE9*/ { "???", IX86_UNKAMD },
  /*0xEA*/ { "???", IX86_UNKAMD },
  /*0xEB*/ { "???", IX86_UNKAMD },
  /*0xEC*/ { "???", IX86_UNKAMD },
  /*0xED*/ { "???", IX86_UNKAMD },
  /*0xEE*/ { "???", IX86_UNKAMD },
  /*0xEF*/ { "???", IX86_UNKAMD },
  /*0xF0*/ { "???", IX86_UNKAMD },
  /*0xF1*/ { "???", IX86_UNKAMD },
  /*0xF2*/ { "???", IX86_UNKAMD },
  /*0xF3*/ { "???", IX86_UNKAMD },
  /*0xF4*/ { "???", IX86_UNKAMD },
  /*0xF5*/ { "???", IX86_UNKAMD },
  /*0xF6*/ { "???", IX86_UNKAMD },
  /*0xF7*/ { "???", IX86_UNKAMD },
  /*0xF8*/ { "???", IX86_UNKAMD },
  /*0xF9*/ { "???", IX86_UNKAMD },
  /*0xFA*/ { "???", IX86_UNKAMD },
  /*0xFB*/ { "???", IX86_UNKAMD },
  /*0xFC*/ { "???", IX86_UNKAMD },
  /*0xFD*/ { "???", IX86_UNKAMD },
  /*0xFE*/ { "???", IX86_UNKAMD },
  /*0xFF*/ { "???", IX86_UNKAMD }
};

#define DECLARE_EX_INSN(n32, n64, func, func64, flags, flags64) { n32, n64, func, func64, flags64, flags }

/*
  note: first column describes XOP_mmmm=08
	second column describes XOP_mmmm=09
*/

const ix86_ExOpcodes ix86_Disassembler::K64_XOP_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x01*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x02*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x03*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x04*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x05*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x06*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x07*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x08*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x09*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x0A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x0B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x0C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x0D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x0E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x0F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x10*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x11*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x12*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x13*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x14*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x15*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x16*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x17*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x18*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x19*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x1A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x1B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x1C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x1D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x1E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x1F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x20*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x21*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x22*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x23*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x24*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x25*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x26*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x27*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x28*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x29*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x2A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x2B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x2C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x2D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x2E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x2F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x30*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x31*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x32*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x33*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x34*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x35*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x36*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x37*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x38*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x39*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x3A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x3B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x3C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x3D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x3E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x3F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x40*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x41*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x42*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x43*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x44*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x45*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x46*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x47*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x48*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x49*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x4A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x4B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x4C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x4D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x4E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x4F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x50*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x51*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x52*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x53*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x54*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x55*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x56*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x57*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x58*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x59*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x5A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x5B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x5C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x5D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x5E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x5F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x60*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x61*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x62*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x63*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x64*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x65*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x66*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x67*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x68*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x69*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x6A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x6B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x6C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x6D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x6E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x6F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x70*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x71*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x72*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x73*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x74*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x75*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x76*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x77*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x78*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x79*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x7A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x7B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x7C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x7D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x7E*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x7F*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x80*/ DECLARE_EX_INSN("vfrczps", "vfrczps", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN("vfrczpd", "vfrczpd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN("vfrczss", "vfrczss", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN("vfrczsd", "vfrczsd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x85*/ DECLARE_EX_INSN("vpmacssww", "vpmacssww", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x86*/ DECLARE_EX_INSN("vpmacsswd", "vpmacsswd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x87*/ DECLARE_EX_INSN("vpmacssdql", "vpmacssdql", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x88*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x89*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x8A*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x8B*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x8C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x8D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x8E*/ DECLARE_EX_INSN("vpmacssdd", "vpmacssdd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x8F*/ DECLARE_EX_INSN("vpmacssdqh", "vpmacssdqh", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x90*/ DECLARE_EX_INSN("vprotb", "vprotb", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x91*/ DECLARE_EX_INSN("vprotw", "vprotw", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x92*/ DECLARE_EX_INSN("vprotd", "vprotd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x93*/ DECLARE_EX_INSN("vprotq", "vprotq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x94*/ DECLARE_EX_INSN("vpshlb", "vpshlb", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_VEX_V|INSN_SSE|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_VEX_V|INSN_SSE|INSN_VEXW_AS_SWAP),
  /*0x95*/ DECLARE_EX_INSN("vpmacsww", "vpshlw", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x96*/ DECLARE_EX_INSN("vpmacswd", "vpshld", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x97*/ DECLARE_EX_INSN("vpmacsdql","vpshlq", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x98*/ DECLARE_EX_INSN("vpshab", "vpshab", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x99*/ DECLARE_EX_INSN("vpshaw", "vpshaw", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9A*/ DECLARE_EX_INSN("vpshad", "vpshad", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9B*/ DECLARE_EX_INSN("vpshaq", "vpshaq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9C*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x9D*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0x9E*/ DECLARE_EX_INSN("vpmacsdd", "vpmacsdd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x9F*/ DECLARE_EX_INSN("vpmacsdqh", "vpmacsdqh", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xA0*/ DECLARE_EX_INSN("vcvtph2ps", "vcvtph2ps", &ix86_Disassembler::arg_simd_imm8, &ix86_Disassembler::arg_simd_imm8, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN("vcvtps2ph", "vcvtps2ph", &ix86_Disassembler::arg_simd_imm8, &ix86_Disassembler::arg_simd_imm8, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN("vpcmov", "vpcmov", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xA3*/ DECLARE_EX_INSN("vpperm", "vpperm", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xA4*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xA5*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xA6*/ DECLARE_EX_INSN("vpmadcsswd", "vpmadcsswd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xA7*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xA8*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xA9*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xAA*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xAB*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xAC*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xAD*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xAE*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xAF*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB0*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB1*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB2*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB3*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB4*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB5*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB6*/ DECLARE_EX_INSN("vpmadcswd", "vpmadcswd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_FAM11|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xB7*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB8*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xB9*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xBA*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xBB*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xBC*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xBD*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xBE*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xBF*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xC0*/ DECLARE_EX_INSN("vprotb", "vprotb", &ix86_Disassembler::arg_simd_imm8, &ix86_Disassembler::arg_simd_imm8, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN("vprotw", "vphaddbw", &ix86_Disassembler::arg_simd_imm8, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("vprotd", "vphaddbd", &ix86_Disassembler::arg_simd_imm8, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC3*/ DECLARE_EX_INSN("vprotq", "vphaddbq", &ix86_Disassembler::arg_simd_imm8, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xC5*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xC6*/ DECLARE_EX_INSN("vphaddwd", "vphaddwd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC7*/ DECLARE_EX_INSN("vphaddwq", "vphaddwq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xC8*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xC9*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xCA*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xCB*/ DECLARE_EX_INSN("vphadddq", "vphadddq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN("vpcomb", "vpcomb",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xCD*/ DECLARE_EX_INSN("vpcomw", "vpcomw",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xCE*/ DECLARE_EX_INSN("vpcomd", "vpcomd",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xCF*/ DECLARE_EX_INSN("vpcomq", "vpcomq",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xD0*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xD1*/ DECLARE_EX_INSN("vphaddubw", "vphaddubw", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD2*/ DECLARE_EX_INSN("vphaddubd", "vphaddubd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD3*/ DECLARE_EX_INSN("vphaddubq", "vphaddubq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD4*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xD5*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xD6*/ DECLARE_EX_INSN("vphadduwd", "vphadduwd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD7*/ DECLARE_EX_INSN("vphadduwq", "vphadduwq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD8*/ DECLARE_EX_INSN("vphaddudq", "vphaddudq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xD9*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xDA*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xDB*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xDC*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xDD*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xDE*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xDF*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE0*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE1*/ DECLARE_EX_INSN("vphsubbw", "vphsubbw", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN("vphsubwd", "vphsubwd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN("vphsubdq", "vphsubdq", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, K64_FAM11|INSN_SSE, K64_FAM11|INSN_SSE),
  /*0xE4*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE5*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE6*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE7*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE8*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xE9*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xEA*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xEB*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xEC*/ DECLARE_EX_INSN("vpcomub", "vpcomub",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xED*/ DECLARE_EX_INSN("vpcomuw", "vpcomuw",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xEE*/ DECLARE_EX_INSN("vpcomud", "vpcomud",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xEF*/ DECLARE_EX_INSN("vpcomuq", "vpcomuq",&ix86_Disassembler::arg_xop_cmp,&ix86_Disassembler::arg_xop_cmp, K64_FAM11|INSN_SSE|INSN_VEX_V, K64_FAM11|INSN_SSE|INSN_VEX_V),
  /*0xF0*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF1*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF2*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF3*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF4*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF5*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF6*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF7*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF8*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xF9*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xFA*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xFB*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xFC*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xFD*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xFE*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11),
  /*0xFF*/ DECLARE_EX_INSN("???", "???", &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, K64_FAM11, K64_FAM11)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_660F01_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN("vmclear", "vmclear", &ix86_Disassembler::ix86_VMX, &ix86_Disassembler::ix86_VMX, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_660F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("pshufb", "pshufb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x01*/ DECLARE_EX_INSN("phaddw", "phaddw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x02*/ DECLARE_EX_INSN("phaddd", "phaddd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x03*/ DECLARE_EX_INSN("phaddsw", "phaddsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x04*/ DECLARE_EX_INSN("pmaddubsw", "pmaddubsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN("phsubw", "phsubw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x06*/ DECLARE_EX_INSN("phsubd", "phsubd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x07*/ DECLARE_EX_INSN("phsubsw", "phsubsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x08*/ DECLARE_EX_INSN("psingb", "psignb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x09*/ DECLARE_EX_INSN("psignw", "psignw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0A*/ DECLARE_EX_INSN("psignd", "psignd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0B*/ DECLARE_EX_INSN("pmulhrsw", "pmulhrsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0C*/ DECLARE_EX_INSN("vpermilps", "vpermilps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x0D*/ DECLARE_EX_INSN("vpermilpd", "vpermilpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x0E*/ DECLARE_EX_INSN("vtestps", "vtestps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x0F*/ DECLARE_EX_INSN("vtestpd", "vtestpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x10*/ DECLARE_EX_INSN("pblendvb", "pblendvb",&ix86_Disassembler::arg_simd_xmm0,&ix86_Disassembler::arg_simd_xmm0, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN("vcvtph2ps", "vcvtph2ps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN("blendvps", "blendvps",&ix86_Disassembler::arg_simd_xmm0,&ix86_Disassembler::arg_simd_xmm0, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x15*/ DECLARE_EX_INSN("blendvpd", "blendvpd",&ix86_Disassembler::arg_simd_xmm0,&ix86_Disassembler::arg_simd_xmm0, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x16*/ DECLARE_EX_INSN("vpermps", "vpermps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x17*/ DECLARE_EX_INSN("ptest", "ptest",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN("vbroadcastss", "vbroadcastss", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x19*/ DECLARE_EX_INSN("vbroadcastsd", "vbroadcastsd", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x1A*/ DECLARE_EX_INSN("vbroadcastf128", "vbroadcastf128", &ix86_Disassembler::arg_simd, &ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN("pabsb", "pabsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN("pabsw", "pabsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN("pabsd", "pabsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P6|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN("pmovsxbw", "pmovsxbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN("pmovsxbd", "pmovsxbd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN("pmovsxbq", "pmovsxbq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN("pmovsxwd", "pmovsxwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN("pmovsxwq", "pmovsxwq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN("pmovsxdq", "pmovsxdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN("pmuldq", "pmuldq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x29*/ DECLARE_EX_INSN("pcmpeqq", "pcmpeqq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2A*/ DECLARE_EX_INSN("movntdqa", "movntdqa",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x2B*/ DECLARE_EX_INSN("packusdw", "packusdw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2C*/ DECLARE_EX_INSN("vmaskmovps", "vmaskmovps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x2D*/ DECLARE_EX_INSN("vmaskmovpd", "vmaskmovpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x2E*/ DECLARE_EX_INSN("vmaskmovps", "vmaskmovps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V|INSN_STORE, INSN_AVX|INSN_VEX_V|INSN_STORE),
  /*0x2F*/ DECLARE_EX_INSN("vmaskmovpd", "vmaskmovpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_AVX|INSN_VEX_V|INSN_STORE, INSN_AVX|INSN_VEX_V|INSN_STORE),
  /*0x30*/ DECLARE_EX_INSN("pmovzxbw", "pmovzxbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN("pmovzxbd", "pmovzxbd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN("pmovzxbq", "pmovzxbq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN("pmovzxwd", "pmovzxwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN("pmovzxwq", "pmovzxwq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN("pmovzxdq", "pmovzxdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN("vpermd", "vpermd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x37*/ DECLARE_EX_INSN("pcmpgtq","pcmpgtq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P7|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN("pminsb", "pminb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x39*/ DECLARE_EX_INSN("pminsd", "pminsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3A*/ DECLARE_EX_INSN("pminuw", "pminuw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3B*/ DECLARE_EX_INSN("pminud", "pminud",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3C*/ DECLARE_EX_INSN("pmaxsb", "pmaxsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3D*/ DECLARE_EX_INSN("pmaxsd", "pmaxsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3E*/ DECLARE_EX_INSN("pmaxuw", "pmaxuw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x3F*/ DECLARE_EX_INSN("pmaxud", "pmaxud",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x40*/ DECLARE_EX_INSN("pmulld", "pmulld",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x41*/ DECLARE_EX_INSN("phminposuw", "phminposuw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN("vpsrlvd", "vpsrlvd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_VEXW_AS_SIZE, INSN_AVX|INSN_VEX_V|INSN_VEXW_AS_SIZE),
  /*0x46*/ DECLARE_EX_INSN("vpsravd", "vpsravd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x47*/ DECLARE_EX_INSN("vpsllvd", "vpsllvd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_VEXW_AS_SIZE, INSN_AVX|INSN_VEX_V|INSN_VEXW_AS_SIZE),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN("vpbroadcastd", "vpbroadcastd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX, INSN_AVX),
  /*0x59*/ DECLARE_EX_INSN("vpbroadcastq", "vpbroadcastq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX, INSN_AVX),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN("vpbroadcastb", "vpbroadcastb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX, INSN_AVX),
  /*0x79*/ DECLARE_EX_INSN("vpbroadcastw", "vpbroadcastw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX, INSN_AVX),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN("invept", "invept", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P7, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN("invvpid", "invvpid", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P7, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN("invpcid", "invpcid", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN("vpmaskmovd", "vpmaskmovd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN("vpmaskmovd", "vpmaskmovd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_STORE, INSN_AVX|INSN_VEX_V|INSN_STORE),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN("vpgatherdd", "vpgatherdd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE, INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE),
  /*0x91*/ DECLARE_EX_INSN("vpgatherqd", "vpgatherqd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE, INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE),
  /*0x92*/ DECLARE_EX_INSN("vgatherdps", "vgatherdps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE, INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE),
  /*0x93*/ DECLARE_EX_INSN("vgatherqps", "vgatherqps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P9|INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE, INSN_AVX|INSN_VEX_V|INSN_VEX_VSIB|INSN_VEXW_AS_SIZE),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN("vfmaddsub132pd", "vfmaddsub132pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x97*/ DECLARE_EX_INSN("vfmsubadd132pd", "vfmsubadd132pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x98*/ DECLARE_EX_INSN("vfmadd132pd", "vfmadd132pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x99*/ DECLARE_EX_INSN("vfmadd132sd", "vfmadd132sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9A*/ DECLARE_EX_INSN("vfmsub132pd", "vfmsub132pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9B*/ DECLARE_EX_INSN("vfmsub132sd", "vfmsub132sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9C*/ DECLARE_EX_INSN("vfnmadd132pd", "vfnmadd132pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9D*/ DECLARE_EX_INSN("vfnmadd132sd", "vfnmadd132sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9E*/ DECLARE_EX_INSN("vfnmsub132pd", "vfnmsub132pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x9F*/ DECLARE_EX_INSN("vfnmsub132sd", "vfnmsub132sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN("vfmaddsub213pd", "vfmaddsub213pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA7*/ DECLARE_EX_INSN("vfmsubadd213pd", "vfmsubadd213pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA8*/ DECLARE_EX_INSN("vfmadd213pd", "vfmadd213pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xA9*/ DECLARE_EX_INSN("vfmadd213sd", "vfmadd213sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAA*/ DECLARE_EX_INSN("vfmsub213pd", "vfmsub213pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAB*/ DECLARE_EX_INSN("vfmsub213sd", "vfmsub213sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAC*/ DECLARE_EX_INSN("vfnmadd213pd", "vfnmadd213pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAD*/ DECLARE_EX_INSN("vfnmadd213sd", "vfnmadd213sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAE*/ DECLARE_EX_INSN("vfnmsub213pd", "vfnmsub213pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xAF*/ DECLARE_EX_INSN("vfnmsub213sd", "vfnmsub213sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN("vfmaddsub231pd", "vfmaddsub231pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB7*/ DECLARE_EX_INSN("vfmsubadd231pd", "vfmsubadd231pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB8*/ DECLARE_EX_INSN("vfmadd231pd", "vfmadd231pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xB9*/ DECLARE_EX_INSN("vfmadd231sd", "vfmadd231sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBA*/ DECLARE_EX_INSN("vfmsub231pd", "vfmsub231pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBB*/ DECLARE_EX_INSN("vfmsub231sd", "vfmsub231sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBC*/ DECLARE_EX_INSN("vfnmadd231pd", "vfnmadd231pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBD*/ DECLARE_EX_INSN("vfnmadd231sd", "vfnmadd231sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBE*/ DECLARE_EX_INSN("vfnmsub231pd", "vfnmsub231pd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xBF*/ DECLARE_EX_INSN("vfnmsub231sd", "vfnmsub231sd",&ix86_Disassembler::arg_fma,&ix86_Disassembler::arg_fma, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN("aesimc", "aesimc", &ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDC*/ DECLARE_EX_INSN("aesenc", "aesenc", &ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDD*/ DECLARE_EX_INSN("aesenclast", "aesenclast", &ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDE*/ DECLARE_EX_INSN("aesdec", "aesdec", &ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xDF*/ DECLARE_EX_INSN("aesdeclast", "aesdeclast", &ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN("shlx", "shlx", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_660F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN("vpermq", "vpermq",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_AVX, INSN_AVX),
  /*0x01*/ DECLARE_EX_INSN("vpermpd", "vpermpd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_AVX, INSN_AVX),
  /*0x02*/ DECLARE_EX_INSN("vpblendd", "vpblendd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN("vpermilps", "vpermilps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x05*/ DECLARE_EX_INSN("vpermilpd", "vpermilpd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x06*/ DECLARE_EX_INSN("vperm2f128", "vperm2f128",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN("roundps", "roundps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN("roundpd", "roundpd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN("roundss", "roundss",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0B*/ DECLARE_EX_INSN("roundsd", "roundsd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0C*/ DECLARE_EX_INSN("blendps", "blendps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0D*/ DECLARE_EX_INSN("blendpd", "blendpd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0E*/ DECLARE_EX_INSN("pblendw", "pblendw",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x0F*/ DECLARE_EX_INSN("palignr", "palignr",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P6|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN("pextrb", "pextrb",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8, IX86_P7|INSN_SSE|INSN_STORE, K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x15*/ DECLARE_EX_INSN("pextrw", "pextrw",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8, IX86_P7|INSN_SSE|INSN_STORE, K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x16*/ DECLARE_EX_INSN("pextrd", "pextrq",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8, IX86_P7|INSN_SSE|INSN_STORE, K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x17*/ DECLARE_EX_INSN("extractps", "extractps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE, K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN("vinsertf128", "vinsertf128",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x19*/ DECLARE_EX_INSN("vextractf128", "vextractf128",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN("vcvtps2ph", "vcvtps2ph",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_STORE, K64_ATHLON|INSN_STORE),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN("pinsrb", "pinsrb",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x21*/ DECLARE_EX_INSN("insertps", "insertps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x22*/ DECLARE_EX_INSN("pinsrd", "pinsrq",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN("vinserti128", "vinserti128",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x39*/ DECLARE_EX_INSN("vextracti128", "vextracti128",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_AVX|INSN_STORE, INSN_AVX|INSN_STORE),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN("dpps", "dpps",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x41*/ DECLARE_EX_INSN("dppd", "dppd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x42*/ DECLARE_EX_INSN("mpsadbw", "mpsadbw",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN("pclmuldq", "pclmuldq",&ix86_Disassembler::arg_simd_clmul,&ix86_Disassembler::arg_simd_clmul, IX86_P8|INSN_AVX, INSN_AVX),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN("vperm2i128", "vperm2i128",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P9|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN("blendvps", "blendvps", &ix86_Disassembler::arg_fma4,&ix86_Disassembler::arg_fma4, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x4B*/ DECLARE_EX_INSN("blendvpd", "blendvpd", &ix86_Disassembler::arg_fma4,&ix86_Disassembler::arg_fma4, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x4C*/ DECLARE_EX_INSN("vpblendvb", "vpblendvb", &ix86_Disassembler::arg_fma4,&ix86_Disassembler::arg_fma4, IX86_P8|INSN_AVX|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN("vfmaddsubps", "vfmaddsubps", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x5D*/ DECLARE_EX_INSN("vfmaddsubpd", "vfmaddsubpd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x5E*/ DECLARE_EX_INSN("vfmsubaddps", "vfmsubaddps", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x5F*/ DECLARE_EX_INSN("vfmsubaddpd", "vfmsubaddpd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x60*/ DECLARE_EX_INSN("pcmpestrm", "pcmpestrm",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x61*/ DECLARE_EX_INSN("pcmpestri", "pcmpestri",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x62*/ DECLARE_EX_INSN("pcmpistrm", "pcmpistrm",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x63*/ DECLARE_EX_INSN("pcmpistri", "pcmpistri",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8, IX86_P7|INSN_SSE|INSN_VEX_V, K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN("vfmaddps", "vfmaddps", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x69*/ DECLARE_EX_INSN("vfmaddpd", "vfmaddpd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6A*/ DECLARE_EX_INSN("vfmaddss", "vfmaddss", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6B*/ DECLARE_EX_INSN("vfmaddsd", "vfmaddsd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6C*/ DECLARE_EX_INSN("vfmsubps", "vfmsubps", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6D*/ DECLARE_EX_INSN("vfmsubpd", "vfmsubpd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6E*/ DECLARE_EX_INSN("vfmsubss", "vfmsubss", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x6F*/ DECLARE_EX_INSN("vfmsubsd", "vfmsubsd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN("vfnmaddps", "vfnmaddps", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x79*/ DECLARE_EX_INSN("vfnmaddpd", "vfnmaddpd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7A*/ DECLARE_EX_INSN("vfnmaddss", "vfnmaddss", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7B*/ DECLARE_EX_INSN("vfnmaddsd", "vfnmaddsd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7C*/ DECLARE_EX_INSN("vfnmsubps", "vfnmsubps", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7D*/ DECLARE_EX_INSN("vfnmsubpd", "vfnmsubpd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7E*/ DECLARE_EX_INSN("vfnmsubss", "vfnmsubss", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x7F*/ DECLARE_EX_INSN("vfnmsubsd", "vfnmsubsd", &ix86_Disassembler::arg_fma4, &ix86_Disassembler::arg_fma4, IX86_P8|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN("aeskeygenassist", "aeskeygenassist", &ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8, IX86_P8|INSN_SSE|INSN_VEX_V, INSN_AVX|INSN_VEX_V),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_660F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x01*/ DECLARE_EX_INSN((const char*)ix86_660F01_Table,(const char*)ix86_660F01_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x10*/ DECLARE_EX_INSN("movupd","movupd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x11*/ DECLARE_EX_INSN("movupd","movupd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movlpd","movlpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x13*/ DECLARE_EX_INSN("movlpd","movlpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x14*/ DECLARE_EX_INSN("unpcklpd","unpcklpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x15*/ DECLARE_EX_INSN("unpckhpd","unpckhpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x16*/ DECLARE_EX_INSN("movhpd","movhpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x17*/ DECLARE_EX_INSN("movhpd","movhpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x28*/ DECLARE_EX_INSN("movapd","movapd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN("movapd","movapd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2A*/ DECLARE_EX_INSN("cvtpi2pd","cvtpi2pd",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntpd","movntpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttpd2pi","cvttpd2pi",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtpd2pi","cvtpd2pi",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_MMX_SSE),
  /*0x2E*/ DECLARE_EX_INSN("ucomisd","ucomisd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN("comisd","comisd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN((const char*)ix86_660F38_Table,(const char*)ix86_660F38_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3A*/ DECLARE_EX_INSN((const char*)ix86_660F3A_Table,(const char*)ix86_660F3A_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x50*/ DECLARE_EX_INSN("movmskpd","movmskpd",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|BRIDGE_CPU_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtpd","sqrtpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x54*/ DECLARE_EX_INSN("andpd","andpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x55*/ DECLARE_EX_INSN("andnpd","andnpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x56*/ DECLARE_EX_INSN("orpd","orpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x57*/ DECLARE_EX_INSN("xorpd","xorpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x58*/ DECLARE_EX_INSN("addpd","addpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulpd","mulpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtpd2ps","cvtpd2ps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN("cvtps2dq","cvtps2dq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5C*/ DECLARE_EX_INSN("subpd","subpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minpd","minpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divpd","divpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxpd","maxpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN("punpcklbw","punpcklbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x61*/ DECLARE_EX_INSN("punpcklwd","punpcklwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x62*/ DECLARE_EX_INSN("punpckldq","punpckldq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x63*/ DECLARE_EX_INSN("packsswb","packsswb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x64*/ DECLARE_EX_INSN("pcmpgtb","pcmpgtb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x65*/ DECLARE_EX_INSN("pcmpgtw","pcmpgtw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x66*/ DECLARE_EX_INSN("pcmpgtd","pcmpgtd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x67*/ DECLARE_EX_INSN("packuswb","packuswb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x68*/ DECLARE_EX_INSN("punpckhbw","punpckhbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x69*/ DECLARE_EX_INSN("punpckhwd","punpckhwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6A*/ DECLARE_EX_INSN("punpckhdq","punpckhdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6B*/ DECLARE_EX_INSN("packssdw","packssdw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6C*/ DECLARE_EX_INSN("punpcklqdq","punpcklqdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6D*/ DECLARE_EX_INSN("punpckhqdq","punpckhqdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x6E*/ DECLARE_EX_INSN("movd","movd",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x6F*/ DECLARE_EX_INSN("movdqa","movdqa",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshufd","pshufd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x71*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgXMMXGr1,&ix86_Disassembler::ix86_ArgXMMXGr1,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x72*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgXMMXGr2,&ix86_Disassembler::ix86_ArgXMMXGr2,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x73*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_ArgXMMXGr3,&ix86_Disassembler::ix86_ArgXMMXGr3,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x74*/ DECLARE_EX_INSN("pcmpeqb","pcmpeqb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x75*/ DECLARE_EX_INSN("pcmpeqw","pcmpeqw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x76*/ DECLARE_EX_INSN("pcmpeqd","pcmpeqd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x78*/ DECLARE_EX_INSN("extrq","extrq",&ix86_Disassembler::arg_simd_rm_imm8_imm8,&ix86_Disassembler::arg_simd_rm_imm8_imm8,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x79*/ DECLARE_EX_INSN("extrq","extrq",&ix86_Disassembler::arg_simd_regrm,&ix86_Disassembler::arg_simd_regrm,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7C*/ DECLARE_EX_INSN("haddpd","haddpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7D*/ DECLARE_EX_INSN("hsubpd","hsubpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7E*/ DECLARE_EX_INSN("movd","movd",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x7F*/ DECLARE_EX_INSN("movdqa","movdqa",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmppd","cmppd",&ix86_Disassembler::arg_simd_cmp,&ix86_Disassembler::arg_simd_cmp,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN("pinsrw","pinsrw",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC5*/ DECLARE_EX_INSN("pextrw","pextrw",&ix86_Disassembler::bridge_simd_cpu_imm8,&ix86_Disassembler::bridge_simd_cpu_imm8,IX86_P4|INSN_SSE|BRIDGE_CPU_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE|BRIDGE_CPU_SSE),
  /*0xC6*/ DECLARE_EX_INSN("shufpd","shufpd",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC7*/ DECLARE_EX_INSN("!!!","!!!",&ix86_Disassembler::ix86_660FVMX,&ix86_Disassembler::ix86_660FVMX,IX86_CPU586,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD0*/ DECLARE_EX_INSN("addsubpd","addsubpd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0xD1*/ DECLARE_EX_INSN("psrlw","psrlw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD2*/ DECLARE_EX_INSN("psrld","psrld",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD3*/ DECLARE_EX_INSN("psrlq","psrlq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD4*/ DECLARE_EX_INSN("paddq","paddq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD5*/ DECLARE_EX_INSN("pmullw","pmullw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD6*/ DECLARE_EX_INSN("movq","movq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0xD7*/ DECLARE_EX_INSN("pmovmskb","pmovmskb",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|BRIDGE_CPU_SSE),
  /*0xD8*/ DECLARE_EX_INSN("psubusb","psubusb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD9*/ DECLARE_EX_INSN("psubusw","psubusw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDA*/ DECLARE_EX_INSN("pminub","pminub",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xDB*/ DECLARE_EX_INSN("pand","pand",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDC*/ DECLARE_EX_INSN("paddusb","paddusb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDD*/ DECLARE_EX_INSN("paddusw","paddusw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDE*/ DECLARE_EX_INSN("pmaxub","pmaxub",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xDF*/ DECLARE_EX_INSN("pandn","pandn",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE0*/ DECLARE_EX_INSN("pavgb","pavgb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE1*/ DECLARE_EX_INSN("psraw","psraw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN("psrad","psrad",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN("pavgw","pavgw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE4*/ DECLARE_EX_INSN("pmulhuw","pmulhuw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE5*/ DECLARE_EX_INSN("pmulhw","pmulhw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE6*/ DECLARE_EX_INSN("cvttpd2dq","cvttpd2dq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE7*/ DECLARE_EX_INSN("movntdq","movntdq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_STORE),
  /*0xE8*/ DECLARE_EX_INSN("psubsb","psubsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE9*/ DECLARE_EX_INSN("psubsw","psubsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEA*/ DECLARE_EX_INSN("pminsw","pminsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEB*/ DECLARE_EX_INSN("por","por",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEC*/ DECLARE_EX_INSN("paddsb","paddsb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xED*/ DECLARE_EX_INSN("paddsw","paddsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEE*/ DECLARE_EX_INSN("pmaxsw","pmaxsw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xEF*/ DECLARE_EX_INSN("pxor","pxor",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF1*/ DECLARE_EX_INSN("psllw","psllw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF2*/ DECLARE_EX_INSN("pslld","pslld",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF3*/ DECLARE_EX_INSN("psllq","psllq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF4*/ DECLARE_EX_INSN("pmuludq","pmuludq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF5*/ DECLARE_EX_INSN("pmaddwd","pmaddwd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF6*/ DECLARE_EX_INSN("psadbw","psadbw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF7*/ DECLARE_EX_INSN("maskmovdqu","maskmovdqu",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xF8*/ DECLARE_EX_INSN("psubb","psubb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xF9*/ DECLARE_EX_INSN("psubw","psubw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFA*/ DECLARE_EX_INSN("psubd","psubd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFB*/ DECLARE_EX_INSN("psubq","psubq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFC*/ DECLARE_EX_INSN("paddb","paddb",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFD*/ DECLARE_EX_INSN("paddw","paddw",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFE*/ DECLARE_EX_INSN("paddd","paddd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_F20F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN("crc32", "crc32",&ix86_Disassembler::arg_cpu_modREGrm,&ix86_Disassembler::arg_cpu_modREGrm, IX86_P7|INSN_OP_BYTE, K64_ATHLON|INSN_OP_BYTE),
  /*0xF1*/ DECLARE_EX_INSN("crc32", "crc32",&ix86_Disassembler::arg_cpu_modREGrm,&ix86_Disassembler::arg_cpu_modREGrm, IX86_P7, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN("pdep", "pdep", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V, K64_ATHLON|INSN_VEX_V),
  /*0xF6*/ DECLARE_EX_INSN("mulx", "mulx", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V, K64_ATHLON|INSN_VEX_V),
  /*0xF7*/ DECLARE_EX_INSN("shrx", "shrx", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_F20F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN("rorx", "rorx",&ix86_Disassembler::arg_cpu_modregrm_imm8,&ix86_Disassembler::arg_cpu_modregrm_imm8, IX86_P9, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_F30F38_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN("pext", "pext", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V, K64_ATHLON|INSN_VEX_V),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN("sarx", "sarx", &ix86_Disassembler::arg_cpu_modregrm, &ix86_Disassembler::arg_cpu_modregrm, IX86_P9|INSN_VEX_V|INSN_VEXW_AS_SWAP, K64_ATHLON|INSN_VEX_V|INSN_VEXW_AS_SWAP),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_F30F3A_Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};



const ix86_ExOpcodes ix86_Disassembler::ix86_F20F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x10*/ DECLARE_EX_INSN("movsd","movsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x11*/ DECLARE_EX_INSN("movsd","movsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movddup","movddup",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtsi2sd","cvtsi2sd",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntsd","movntsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P7|INSN_SSE|INSN_STORE,K64_FAM10|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttsd2si","cvttsd2si",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtsd2si","cvtsd2si",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P4|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN((const char*)ix86_F20F38_Table,(const char*)ix86_F20F38_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3A*/ DECLARE_EX_INSN((const char*)ix86_F20F3A_Table,(const char*)ix86_F20F3A_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtsd","sqrtsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x58*/ DECLARE_EX_INSN("addsd","addsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulsd","mulsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtsd2ss","cvtsd2ss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x5C*/ DECLARE_EX_INSN("subsd","subsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minsd","minsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divsd","divsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxsd","maxsd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshuflw","pshuflw",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x78*/ DECLARE_EX_INSN("insertq","insertq",&ix86_Disassembler::arg_simd_regrm_imm8_imm8,&ix86_Disassembler::arg_simd_regrm_imm8_imm8,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x79*/ DECLARE_EX_INSN("insertq","insertq",&ix86_Disassembler::arg_simd_regrm,&ix86_Disassembler::arg_simd_regrm,IX86_P7|INSN_SSE,K64_FAM10|INSN_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7C*/ DECLARE_EX_INSN("haddps","haddps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7D*/ DECLARE_EX_INSN("hsubps","hsubps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_FAM9|INSN_SSE|INSN_VEX_V),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmpsd","cmpsd",&ix86_Disassembler::arg_simd_cmp,&ix86_Disassembler::arg_simd_cmp,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD0*/ DECLARE_EX_INSN("addsubps","addsubps",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movdq2q","movdq2q",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P4|INSN_SSE|BRIDGE_MMX_SSE,K64_ATHLON|INSN_SSE|BRIDGE_MMX_SSE),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvtpd2dq","cvtpd2dq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF0*/ DECLARE_EX_INSN("lddqu","lddqu",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE)
};

const ix86_ExOpcodes ix86_Disassembler::ix86_F30F_PentiumTable[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x10*/ DECLARE_EX_INSN("movss","movss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x11*/ DECLARE_EX_INSN("movss","movss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_VEX_V|INSN_STORE),
  /*0x12*/ DECLARE_EX_INSN("movsldup","movsldup",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x16*/ DECLARE_EX_INSN("movshdup","movshdup",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P5|INSN_SSE,K64_FAM9|INSN_SSE),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2A*/ DECLARE_EX_INSN("cvtsi2ss","cvtsi2ss",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x2B*/ DECLARE_EX_INSN("movntss","movntss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P7|INSN_SSE|INSN_STORE,K64_FAM10|INSN_SSE|INSN_STORE),
  /*0x2C*/ DECLARE_EX_INSN("cvttss2si","cvttss2si",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2D*/ DECLARE_EX_INSN("cvtss2si","cvtss2si",&ix86_Disassembler::bridge_simd_cpu,&ix86_Disassembler::bridge_simd_cpu,IX86_P3|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE,K64_ATHLON|INSN_SSE|INSN_VEX_V|BRIDGE_CPU_SSE),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x38*/ DECLARE_EX_INSN((const char*)ix86_F30F38_Table,(const char*)ix86_F30F38_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3A*/ DECLARE_EX_INSN((const char*)ix86_F30F3A_Table,(const char*)ix86_F30F3A_Table,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,TAB_NAME_IS_TABLE,TAB_NAME_IS_TABLE),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x51*/ DECLARE_EX_INSN("sqrtss","sqrtss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x52*/ DECLARE_EX_INSN("rsqrtss","rsqrtss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x53*/ DECLARE_EX_INSN("rcpss","rcpss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x58*/ DECLARE_EX_INSN("addss","addss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x59*/ DECLARE_EX_INSN("mulss","mulss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5A*/ DECLARE_EX_INSN("cvtss2sd","cvtss2sd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5B*/ DECLARE_EX_INSN("cvttps2dq","cvttps2dq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5C*/ DECLARE_EX_INSN("subss","subss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5D*/ DECLARE_EX_INSN("minss","minss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5E*/ DECLARE_EX_INSN("divss","divss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x5F*/ DECLARE_EX_INSN("maxss","maxss",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x6F*/ DECLARE_EX_INSN("movdqu","movdqu",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x70*/ DECLARE_EX_INSN("pshufhw","pshufhw",&ix86_Disassembler::arg_simd_imm8,&ix86_Disassembler::arg_simd_imm8,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x7E*/ DECLARE_EX_INSN("movq","movq",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0x7F*/ DECLARE_EX_INSN("movdqu","movdqu",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_STORE,K64_ATHLON|INSN_SSE|INSN_STORE),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xAE*/ DECLARE_EX_INSN("!!!", "!!!", &ix86_Disassembler::ix86_ArgFsGsBaseGrp,&ix86_Disassembler::ix86_ArgFsGsBaseGrp,IX86_P9,K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xB8*/ DECLARE_EX_INSN("popcnt","popcnt",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P7,K64_FAM10),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBC*/ DECLARE_EX_INSN("tzcnt","tzcnt",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P9,K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN("lzcnt","lzcnt",&ix86_Disassembler::arg_cpu_modregrm,&ix86_Disassembler::arg_cpu_modregrm,IX86_P7,K64_FAM10),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC2*/ DECLARE_EX_INSN("cmpss","cmpss",&ix86_Disassembler::arg_simd_cmp,&ix86_Disassembler::arg_simd_cmp,IX86_P3|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC7*/ DECLARE_EX_INSN("vmxon","vmxon",&ix86_Disassembler::arg_cpu_mod_rm,&ix86_Disassembler::arg_cpu_mod_rm,IX86_P6,K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD6*/ DECLARE_EX_INSN("movq2dq","movq2dq",&ix86_Disassembler::bridge_sse_mmx,&ix86_Disassembler::bridge_sse_mmx,IX86_P4|INSN_SSE,K64_ATHLON|INSN_SSE),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE6*/ DECLARE_EX_INSN("cvtdq2pd","cvtdq2pd",&ix86_Disassembler::arg_simd,&ix86_Disassembler::arg_simd,IX86_P4|INSN_SSE|INSN_VEX_V,K64_ATHLON|INSN_SSE|INSN_VEX_V),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL,&ix86_Disassembler::ix86_Null,&ix86_Disassembler::ix86_Null,IX86_UNKMMX,K64_ATHLON|INSN_SSE)
};

#if 0
const ix86_Opcodes ix86_Disassembler::ix86_NullTable[256] =
{
  /*0x00*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x01*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x02*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x03*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x04*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x05*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x06*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x07*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x08*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x09*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x0A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x0B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x0C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x0D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x0E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x0F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x10*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x11*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x12*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x13*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x14*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x15*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x16*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x17*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x18*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x19*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x1A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x1B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x1C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x1D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x1E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x1F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x20*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x21*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x22*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x23*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x24*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x25*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x26*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x27*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x28*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x29*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x2A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x2B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x2C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x2D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x2E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x2F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x30*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x31*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x32*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x33*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x34*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x35*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x36*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x37*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x38*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x39*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x3A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x3B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x3C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x3D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x3E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x3F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x40*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x41*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x42*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x43*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x44*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x45*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x46*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x47*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x48*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x49*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x4A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x4B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x4C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x4D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x4E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x4F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x50*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x51*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x52*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x53*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x54*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x55*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x56*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x57*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x58*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x59*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x5A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x5B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x5C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x5D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x5E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x5F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x60*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x61*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x62*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x63*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x64*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x65*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x66*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x67*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x68*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x69*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x6A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x6B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x6C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x6D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x6E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x6F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x70*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x71*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x72*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x73*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x74*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x75*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x76*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x77*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x78*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x79*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x7A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x7B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x7C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x7D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x7E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x7F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x80*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x81*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x82*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x83*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x84*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x85*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x86*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x87*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x88*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x89*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x8A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x8B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x8C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x8D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x8E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x8F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x90*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x91*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x92*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x93*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x94*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x95*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x96*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x97*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x98*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x99*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x9A*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x9B*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x9C*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x9D*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x9E*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0x9F*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA0*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA1*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA2*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA3*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA4*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA5*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA6*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA7*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA8*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xA9*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xAA*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xAB*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xAC*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xAD*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xAE*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xAF*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB0*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB1*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB2*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB3*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB4*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB5*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB6*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB7*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB8*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xB9*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xBA*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xBB*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xBC*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xBD*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xBE*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xBF*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC0*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC1*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC2*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC3*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC4*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC5*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC6*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC7*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC8*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xC9*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xCA*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xCB*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xCC*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xCD*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xCE*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xCF*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD0*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD1*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD2*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD3*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD4*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD5*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD6*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD7*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD8*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xD9*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xDA*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xDB*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xDC*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xDD*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xDE*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xDF*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE0*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE1*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE2*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE3*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE4*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE5*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE6*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE7*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE8*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xE9*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xEA*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xEB*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xEC*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xED*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xEE*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xEF*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF0*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF1*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF2*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF3*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF4*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF5*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF6*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF7*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF8*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xF9*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xFA*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xFB*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xFC*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xFD*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xFE*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU },
  /*0xFF*/ { "???", "???", &ix86_Disassembler::ix86_Null, IX86_UNKCPU }
};

const ix86_ExOpcodes ix86_Disassembler::ix86_Null64Table[256] =
{
  /*0x00*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x01*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x02*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x03*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x04*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x05*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x06*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x07*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x08*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x09*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x0F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x10*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x11*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x12*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x13*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x14*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x15*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x16*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x17*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x18*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x19*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x1F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x20*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x21*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x22*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x23*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x24*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x25*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x26*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x27*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x28*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x29*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x2F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x30*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x31*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x32*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x33*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x34*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x35*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x36*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x37*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x38*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x39*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x3F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x40*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x41*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x42*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x43*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x44*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x45*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x46*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x47*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x48*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x49*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x4F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x50*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x51*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x52*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x53*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x54*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x55*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x56*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x57*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x58*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x59*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x5F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x60*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x61*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x62*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x63*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x64*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x65*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x66*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x67*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x68*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x69*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x6F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x70*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x71*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x72*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x73*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x74*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x75*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x76*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x77*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x78*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x79*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x7F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x80*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x81*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x82*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x83*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x84*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x85*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x86*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x87*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x88*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x89*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x8F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x90*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x91*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x92*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x93*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x94*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x95*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x96*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x97*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x98*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x99*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9A*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9B*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9C*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9D*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9E*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0x9F*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xA9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xAF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xB9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xBF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xC9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xCF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xD9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xDF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xE9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xED*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xEF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF0*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF1*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF2*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF3*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF4*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF5*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF6*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF7*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF8*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xF9*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFA*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFB*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFC*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFD*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFE*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON),
  /*0xFF*/ DECLARE_EX_INSN(NULL, NULL, &ix86_Disassembler::ix86_Null, &ix86_Disassembler::ix86_Null, IX86_UNKCPU, K64_ATHLON)
};
#endif

const char* ix86_Disassembler::ix86_Op1Names[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
const char* ix86_Disassembler::ix86_ShNames[] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "RESERVED", "sar" };
const char* ix86_Disassembler::ix86_Gr1Names[] = { "test", "RESERVED", "not", "neg", "mul", "imul", "div", "idiv" };
const char* ix86_Disassembler::ix86_Gr2Names[] = { "inc", "dec", "call", "call", "jmp", "jmp", "push", "???"/** pop - not real */};

const char* ix86_Disassembler::ix86_ExGrp0[] = { "sldt", "str", "lldt", "ltr", "verr", "verw", "???", "???" };

const char* ix86_Disassembler::ix86_BitGrpNames[] = { "???", "???", "???", "???", "bt", "bts", "btr", "btc" };

const char* ix86_Disassembler::ix86_MMXGr1[] = { "???", "???", "psrlw", "???", "psraw", "???", "psllw", "???" };
const char* ix86_Disassembler::ix86_MMXGr2[] = { "???", "???", "psrld", "???", "psrad", "???", "pslld", "???" };
const char* ix86_Disassembler::ix86_MMXGr3[] = { "???", "???", "psrlq", "???", "???",   "???", "psllq", "???" };

const char* ix86_Disassembler::ix86_XMMXGr1[] = { "???", "???", "psrlw", "???", "psraw", "???", "psllw", "???" };
const char* ix86_Disassembler::ix86_XMMXGr2[] = { "???", "???", "psrld", "???", "psrad", "???", "pslld", "???" };
const char* ix86_Disassembler::ix86_XMMXGr3[] = { "???", "???", "psrlq", "psrldq", "???",   "???", "psllq", "pslldq" };

const char* ix86_Disassembler::ix86_3dPrefetchGrp[] = { "prefetch", "prefetchw", "prefetch", "prefetch", "prefetch", "prefetch", "prefetch", "prefetch" };

const char* ix86_Disassembler::ix86_KatmaiGr2Names[] = { "prefetchnta", "prefetcht0", "prefetcht1", "prefetcht2", "???", "???", "???", "???" };

Bin_Format::bitness ix86_Disassembler::BITNESS = Bin_Format::Auto;

char ix86_Disassembler::ix86_segpref[4] = "";

const unsigned char ix86_Disassembler::leave_insns[] = { 0x07, 0x17, 0x1F, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x61, 0x90, 0xC9 };

bool ix86_Disassembler::is_listed(unsigned char insn,const unsigned char *list,size_t listsize) const
{
  size_t i;
  for(i = 0;i < listsize;i++) if(insn == list[i]) return true;
  return false;
}

/*
  For long 64 mode:
  [legacy prefix] [REX] [Opcode] ...
  legacy prefixes: 26 2E 36 3E 64 65 66 67 F0 F3 F2
*/
MBuffer ix86_Disassembler::parse_REX_type(MBuffer insn,char *up,ix86Param& DisP) const
{
    if(x86_Bitness == Bin_Format::Use64 && !(DisP.pfx&PFX_REX)) {
	DisP.pfx|=PFX_REX;
	DisP.REX = insn[0];
    }
    (*up)++;
    return &insn[1];
}

void ix86_Disassembler::ix86_gettype(DisasmRet *dret,ix86Param& _DisP) const
{
 MBuffer insn;
 char ua,ud,up,has_lock,has_rep,has_seg,has_vex;
 insn = &_DisP.RealCmd[0];
 dret->pro_clone = __INSNT_ORDINAL;
 has_vex = false;
 has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
 RepeateByPrefix:
 if(x86_Bitness == Bin_Format::Use64)
 {
   if(ua+has_seg+has_rep+has_lock+has_vex>4) goto get_type;
   if(ud>6) goto get_type;
 }
 else
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 6 || has_vex > 0) goto get_type;
 /** do prefixes loop */
 switch(insn[0])
 {
   case 0x40:
   case 0x41:
   case 0x42:
   case 0x43:
   case 0x44:
   case 0x45:
   case 0x46:
   case 0x47:
   case 0x48:
   case 0x49:
   case 0x4A:
   case 0x4B:
   case 0x4C:
   case 0x4D:
   case 0x4E:
   case 0x4F:
		if(x86_Bitness == Bin_Format::Use64) {
		    insn=parse_REX_type(insn,&up,_DisP);
		    goto RepeateByPrefix;
		}
		else goto MakePref;
		break;
   case 0x26:
   case 0x2E:
   case 0x36:
   case 0x3E:
	      do_seg:
	      if(has_seg) break;
	      has_seg++;
	      goto MakePref;
   case 0x64:
   case 0x65:
		if(x86_Bitness != Bin_Format::Use64) goto do_seg;
		goto MakePref;
   case 0x66:
		_DisP.pfx|=PFX_66;
	      ud++;
	      goto MakePref;
   case 0x67:
		_DisP.pfx|=PFX_67;
	      ua++;
	      goto MakePref;
   case 0xF0:
		_DisP.pfx|=PFX_LOCK;
	      if(has_lock + has_rep) break;
	      has_lock++;
	      goto MakePref;
   case 0x8F:
   case 0xC4:
	      if(x86_Bitness == Bin_Format::Use64) has_vex++;
	      else if((insn[1]&0xC0)==0xC0) has_vex++;
	      insn=&insn[2];
   case 0xC5:
	      if(x86_Bitness == Bin_Format::Use64) has_vex++;
	      else if((insn[1]&0xC0)==0xC0) has_vex++;
	      insn=&insn[1];
	      goto MakePref;
   case 0xF2:
		_DisP.pfx|=PFX_F2_REPNE;
	      if(has_rep + has_lock)  break;
	      has_rep++;
	      goto MakePref;
   case 0xF3:
	      if(insn[1] == 0x90) goto get_type;
		_DisP.pfx|=PFX_F3_REP;
	      if(has_rep + has_lock) break;
	      has_rep++;
	      goto MakePref;
   default:   break;
   MakePref:
	      insn = &insn[1];
	      up++;
	      goto RepeateByPrefix;
 }
  /** First check for SSE extensions */
  if(insn[0] == 0x0F && (_DisP.pfx&(PFX_66|PFX_F2_REPNE|PFX_F3_REP))) {
    const ix86_ExOpcodes *SSE2_ext=ix86_extable;
    if(_DisP.pfx&PFX_F2_REPNE) SSE2_ext=ix86_F20F_PentiumTable;
    else
    if(_DisP.pfx&PFX_F3_REP)   SSE2_ext=ix86_F30F_PentiumTable;
    else
    if(_DisP.pfx&PFX_66)       SSE2_ext=ix86_660F_PentiumTable;
    if(!(
		x86_Bitness==Bin_Format::Use64?
		SSE2_ext[insn[1]].name64:
		SSE2_ext[insn[1]].name
		))  return;
  }
 get_type:
  if(x86_Bitness == Bin_Format::Use64)
  {
    _DisP.mode|= MOD_WIDE_ADDR;
    if(_DisP.pfx&PFX_REX && ((_DisP.REX & 0x0F)>>3) != 0) _DisP.mode|=MOD_WIDE_DATA;
  }
   if((insn[0] & 0xF6) == 0xC2)
   {
     dret->pro_clone = __INSNT_RET;
     make_RET:
     if((insn[0] & 0xC7) == 0xC3)
     {
       dret->codelen = 0;
       dret->field   = 0;
     }
     else
     {
       dret->codelen = 2;
       dret->field   = 2;
     }
     return; /* Return here immediatly, because we have 'goto' operator. */
   }
     else
     if(insn[0] == 0xFF && (insn[1]==0x15 || insn[1] == 0x25) && x86_Bitness == Bin_Format::Use64)
     {
	dret->pro_clone = __INSNT_JMPRIP;
	dret->codelen = 4;
	dret->field = 2;
     }
     else
     if(insn[0] == 0xFF &&
	(((insn[1] & 0x27) == 0x25 && (_DisP.mode&MOD_WIDE_ADDR)) ||
	((insn[1] & 0x27) == 0x26 && !(_DisP.mode&MOD_WIDE_ADDR))))
     {
	dret->pro_clone = __INSNT_JMPVVT;
	dret->codelen = (insn[1] & 0x27) == 0x25 ? 4 : 2;
	dret->field = 2;
     }
     else
     if(insn[0] == 0xFF && insn[1] == 0xA3 && (_DisP.mode&MOD_WIDE_ADDR))
     {
	dret->pro_clone = __INSNT_JMPPIC;
	dret->codelen = 4;
	dret->field = 2;
     }
     else
     {
	/* Attempt determine leave insn */
	size_t i;
	bool leave_cond = false, ret_reached = false;
	for(i = 0;i < (PREDICT_DEPTH-1)*MAX_IX86_INSN_LEN;i++)
	{
	  /*
	     RETURN insn can not be reached first because it directly checked
	     above. Therefore 'leave_cond' variable will computed correctly.
	  */
	  if((insn[i] & 0xF6) == 0xC2) { ret_reached = true; break; }
	  leave_cond = is_listed(insn[i],leave_insns,sizeof(leave_insns));
	  if(!leave_cond)
	  {
	    if((insn[i] == 0x89 && insn[i+1] == 0xC9) || /* mov esp, ebp */
	       (insn[i] == 0x8B && insn[i+1] == 0xE5) || /* mov esp, ebp */
	       (insn[i] == 0xD9 && insn[i+1] == 0xD0) || /* fnop */
	       (insn[i] == 0xDD && insn[i+1] == 0xD8) || /* fstp st(0), st(0) */
	       (insn[i] == 0x0F && insn[i+1] == 0x77)    /* emms */
	      )
	    {
	      leave_cond = true;
	      i++;
	    }
	    else
	    if(insn[i] == 0x83 && (insn[i+1] == 0xC4 || insn[i+1] == 0xEC))
	    { /* sub/add esp, short_num */
	      leave_cond = true;
	      i += 2;
	    }
	    else
	    if(insn[i] == 0x81 && (insn[i+1] == 0xC4 || insn[i+1] == 0xEC))
	    { /* sub/add esp, long_num */
	      leave_cond = true;
	      i += (_DisP.mode&MOD_WIDE_DATA) ? 5 : 3;
	    }
	  }
	  if(!leave_cond) break;
	}
	if(leave_cond && ret_reached)
	{
	  dret->pro_clone = __INSNT_LEAVE;
	  insn = &insn[i];
	  goto make_RET;
	}
     }
}

unsigned char ix86_Disassembler::parse_REX(unsigned char code,ix86Param& DisP) const
{
    if(x86_Bitness == Bin_Format::Use64 && !(DisP.pfx&PFX_REX))
    {
	DisP.pfx|=PFX_REX;
	DisP.REX = code;
    }
    DisP.CodeAddress++;
    DisP.RealCmd = &DisP.RealCmd[1];
    return DisP.RealCmd[0];
}

void ix86_Disassembler::parse_VEX_pp(ix86Param& DisP) const {
    if((DisP.VEX_vlp & 0x03) == 0x01) DisP.pfx|=PFX_66;
    else
    if((DisP.VEX_vlp & 0x03) == 0x02) DisP.pfx|=PFX_F3_REP;
    else
    if((DisP.VEX_vlp & 0x03) == 0x03) DisP.pfx|=PFX_F2_REPNE;
}

void ix86_Disassembler::parse_VEX_C4(ix86Param& DisP) const {
  unsigned char code;
  DisP.pfx|=PFX_VEX;
  DisP.pfx|=PFX_REX;
  code=DisP.RealCmd[1];
  DisP.REX=0x40;
  DisP.VEX_m = code&0x1F;
  DisP.REX|=((code>>5)&0x01);    /* make B */
  DisP.REX|=((code>>6)&0x01)<<1; /* make X */
  DisP.REX|=((code>>7)&0x01)<<2; /* make R */
  code=DisP.RealCmd[2];
  DisP.REX|=((code>>7)&0x01)<<3; /* make W */
  DisP.REX^=0x0F;                /* complenent it */
  DisP.VEX_vlp=code&0x7F;
  parse_VEX_pp(DisP);
}

void ix86_Disassembler::parse_VEX_C5(ix86Param& DisP) const {
  unsigned char code;
  DisP.pfx|=PFX_VEX;
  DisP.pfx|=PFX_REX;
  DisP.REX=0x4F;
  code=DisP.RealCmd[1];
  DisP.REX|=((code>>7)&0x01)<<2; /* make R */
  DisP.REX^=0x0F;                /* complenent it */
  DisP.VEX_m = 0x01; /* Fake 0Fh opcode*/
  DisP.VEX_vlp=code&0x7F;
  parse_VEX_pp(DisP);
}

void ix86_Disassembler::parse_XOP_8F(ix86Param& DisP) const {
    parse_VEX_C4(DisP);
    DisP.pfx|=PFX_XOP;
    DisP.XOP_m = DisP.VEX_m;
    DisP.VEX_m = 0x01; /* Fake 0Fh opcode*/
}

DisasmRet ix86_Disassembler::disassembler(__filesize_t ulShift,
					MBuffer buffer,
					unsigned flags)
{
 unsigned char code;
 DisasmRet Ret;
 ix86Param DisP;
 unsigned char ua,ud,up;
 char has_lock,has_rep,has_seg,has_vex,has_xop;
 bool has_rex;

 memset(&DisP,0,sizeof(DisP));
 if(x86_Bitness == Bin_Format::Use64) DisP.pro_clone=K64_ATHLON;
 else DisP.pro_clone = IX86_CPU086;
 DisP.codelen = 1;
 DisP.DisasmPrefAddr = ulShift;
 DisP.CodeAddress = ulShift;
 DisP.CodeBuffer = buffer;
 DisP.RealCmd = buffer;
 DisP.flags = flags;
 /*
    __DISF_GETTYPE must be reentrance. Must do not modify any internal
    disassembler variables and flags and e.t.c.
    It is calling from disassembler recursively.
 */
 if(flags & __DISF_GETTYPE)
 {
   ix86_gettype(&Ret,DisP);
   goto Bye;
 }

 ix86_segpref[0] = 0;
 has_xop = has_rex = has_vex = false;
 has_lock = has_rep = has_seg = 0;
 up = ua = ud = 0;
 ix86_da_out[0] = 0;

 if(BITNESS == Bin_Format::Auto) x86_Bitness = bin_format.query_bitness(ulShift);
 else x86_Bitness = BITNESS;

 if(x86_Bitness > Bin_Format::Use16)
 {
    DisP.mode|=MOD_WIDE_DATA;
    DisP.mode|=MOD_WIDE_ADDR;
 }

 ix86_str[0] = 0;
 RepeateByPrefix:
 code = DisP.RealCmd[0];
 if(x86_Bitness == Bin_Format::Use64)
 {
   if(ua+has_seg+has_rep+has_lock+has_vex+has_xop>4) goto bad_prefixes;
   /* it enables largest NOP generated by gcc:
      66 66 66 66 66 66 2E 0F 1F 84 00 00 00 00 00
   */
   if(ud>6) goto bad_prefixes;
 }
 else
 if(has_lock + has_rep > 1 || has_seg > 1 || ua > 1 || ud > 6 || has_vex > 1 || has_rex > 1 || has_xop > 1)
 {
   bad_prefixes:
   DisP.codelen = ud;
   strcpy(ix86_str,"???");
   goto ExitDisAsm;
 }
 if(has_vex || has_xop) goto end_of_prefixes;
 /** do prefixes loop */
 switch(code)
 {
   case 0x40:
   case 0x41:
   case 0x42:
   case 0x43:
   case 0x44:
   case 0x45:
   case 0x46:
   case 0x47:
   case 0x48:
   case 0x49:
   case 0x4A:
   case 0x4B:
   case 0x4C:
   case 0x4D:
   case 0x4E:
   case 0x4F: if(x86_Bitness == Bin_Format::Use64 && !(DisP.pfx&PFX_REX))
	      {
		has_rex=1;
		code=parse_REX(code,DisP);
		up++;
		goto RepeateByPrefix;
	      }
	      break;
   case 0x26:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_ES;
		strcpy(ix86_segpref,"es:");
		has_seg++;
		goto MakePref;
   case 0x2E:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_CS;
		strcpy(ix86_segpref,"cs:");
		has_seg++;
		goto MakePref;
   case 0x36:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_SS;
		strcpy(ix86_segpref,"ss:");
		has_seg++;
		goto MakePref;
   case 0x3E:
		if(has_seg) break;
		/*in 64-bit mode, the CS,DS,ES,SS segment overrides are ignored but may be specified */
		DisP.pfx|=PFX_SEG_DS;
		strcpy(ix86_segpref,"ds:");
		has_seg++;
		goto MakePref;
   case 0x64:
		if(has_seg) break;
		DisP.pfx|=PFX_SEG_FS;
		strcpy(ix86_segpref,"fs:");
		has_seg++;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x65:
		if(has_seg) break;
		DisP.pfx|=PFX_SEG_GS;
		strcpy(ix86_segpref,"gs:");
		has_seg++;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x66:
		ud++;
		DisP.pfx |= PFX_66;
		DisP.mode ^= MOD_WIDE_DATA;
		DisP.pro_clone = IX86_CPU386;
		goto MakePref;
   case 0x67:
		ua++;
		DisP.pfx |= PFX_67;
		DisP.mode ^= MOD_WIDE_ADDR;
		goto MakePref;
   case 0x8F:
		if(x86_Bitness == Bin_Format::Use64) has_xop++;
		else if((DisP.RealCmd[1]&0xC0)==0xC0) has_xop++;
		if(has_xop) {
		    parse_XOP_8F(DisP);
		    DisP.CodeAddress+=2;
		    up+=2;
		    DisP.RealCmd = &DisP.RealCmd[2];
		    goto MakePref;
		}
		break;
   case 0xC4:
		if(x86_Bitness == Bin_Format::Use64) has_vex++;
		else if((DisP.RealCmd[1]&0xC0)==0xC0) has_vex++;
		if(has_vex) {
		    parse_VEX_C4(DisP);
		    DisP.CodeAddress+=2;
		    up+=2;
		    DisP.RealCmd = &DisP.RealCmd[2];
		    goto MakePref;
		}
		break;
   case 0xC5:
		if(x86_Bitness == Bin_Format::Use64) has_vex++;
		else if((DisP.RealCmd[1]&0xC0)==0xC0) has_vex++;
		if(has_vex) {
		    parse_VEX_C5(DisP);
		    DisP.CodeAddress++;
		    up++;
		    DisP.RealCmd = &DisP.RealCmd[1];
		    goto MakePref;
		}
		break;
   case 0xF0:
		if(has_lock + has_rep) break;
		has_lock++;
		DisP.pfx|=PFX_LOCK;
		strcat(ix86_da_out,"lock; ");
		goto MakePref;
   case 0xF2:
		DisP.pfx|=PFX_F2_REPNE;
		if(has_rep + has_lock) break;
		has_rep++;
		strcat(ix86_da_out,"repne; ");
		goto MakePref;
   case 0xF3:
		DisP.pfx|=PFX_F3_REP;
		if(DisP.RealCmd[1] == 0x90) {
		/* this is pause insns */
		    strcpy(ix86_str,x86_Bitness == Bin_Format::Use64?"pause":"pause");
		    DisP.codelen++;
		    DisP.pro_clone = Bin_Format::Use64?K64_ATHLON:IX86_P4;
		    goto ExitDisAsm;
		}
		if(has_rep + has_lock) break;
		has_rep++;
		strcat(ix86_da_out,"rep; ");
		goto MakePref;
   default:   break;
   MakePref:
	      DisP.CodeAddress++;
	      DisP.RealCmd = &DisP.RealCmd[1];
	      code = DisP.RealCmd[0];
	      up++;
	      goto RepeateByPrefix;
 }
 end_of_prefixes:
 /* Let it be overwritten later */
 if(x86_Bitness == Bin_Format::Use64) DisP.insn_flags = ix86_table[code].flags64;
 else
 DisP.insn_flags = ix86_table[code].pro_clone;
 if((DisP.pfx&PFX_VEX) && DisP.VEX_m>0 && DisP.VEX_m<4 && !(DisP.pfx&PFX_XOP)) goto fake_0F_opcode;
 if(code==0x0F && (DisP.pfx&(PFX_F2_REPNE|PFX_F3_REP|PFX_66))) {
    const ix86_ExOpcodes *SSE2_ext;
    unsigned char ecode;
    const char *nam;
    ix86_method mthd;
    /* for continuing  */
    unsigned char up_saved;
    ix86Param DisP_saved;
    fake_0F_opcode:
    DisP_saved=DisP;
    up_saved=up;
    SSE2_ext=ix86_extable;
    if(DisP.pfx&PFX_F2_REPNE) SSE2_ext=ix86_F20F_PentiumTable;
    else
    if(DisP.pfx&PFX_F3_REP)   SSE2_ext=ix86_F30F_PentiumTable;
    else
    if(DisP.pfx&PFX_66)       SSE2_ext=ix86_660F_PentiumTable;
    ecode = (DisP.pfx&PFX_VEX)?DisP.RealCmd[0]:DisP.RealCmd[1];
    if((DisP.pfx&PFX_VEX) && DisP.VEX_m==0x02) ecode = 0x38;
    else
    if((DisP.pfx&PFX_VEX) && DisP.VEX_m==0x03) ecode = 0x3A;

    SSE2_ext=ix86_prepare_flags(SSE2_ext,DisP,&ecode,&up);

    if((DisP.pfx&PFX_VEX) && DisP.VEX_m>1) {
	DisP.RealCmd=&DisP.RealCmd[-1];
	up--;
    }
    if(DisP.pfx&PFX_VEX)	ecode=DisP.RealCmd[0];
    else			ecode=DisP.RealCmd[1];

    nam=((x86_Bitness==Bin_Format::Use64)?SSE2_ext[ecode].name64:SSE2_ext[ecode].name);
    if(nam) {
	if(DisP.pfx&PFX_66&&
	    (SSE2_ext==ix86_660F_PentiumTable||
	     SSE2_ext==ix86_660F38_Table||
	     SSE2_ext==ix86_660F3A_Table
	    )) DisP.mode|=MOD_WIDE_DATA;
	if(DisP.pfx&PFX_VEX && nam[0]!='v') {
	    strcpy(ix86_str,"v");
	    strcat(ix86_str,nam);
	}
	else strcpy(ix86_str,nam);
	ix86_da_out[0]='\0'; /* disable rep; lock; prefixes */
	if(!(DisP.pfx&PFX_VEX) && (DisP.pfx&(PFX_F2_REPNE|PFX_F3_REP|PFX_66))) {
	    DisP.RealCmd = &DisP.RealCmd[1];
	    up++;
	}
	if(x86_Bitness == Bin_Format::Use64)	DisP.insn_flags = SSE2_ext[ecode].flags64;
	else				DisP.insn_flags = SSE2_ext[ecode].pro_clone;
	mthd=((x86_Bitness==Bin_Format::Use64)?SSE2_ext[ecode].method64:SSE2_ext[ecode].method);
	if(mthd) {
		TabSpace(ix86_str,TAB_POS);
		(this->*mthd)(ix86_str,DisP);
	}
	goto ExitDisAsm;
    }
    /* continue ordinal execution */
    DisP=DisP_saved;
    up=up_saved;
 }
 if(DisP.pfx&PFX_XOP) {
    const ix86_ExOpcodes* _this = &K64_XOP_Table[code];
    if(DisP.XOP_m==0x08)	strcpy(ix86_str,_this->name); /* emulate 8F.08 */
    else			strcpy(ix86_str,_this->name64);
    if(DisP.XOP_m==0x08 && _this->method) { /* emulate 8F.08 */
	ix86_method mtd = _this->method;
	TabSpace(ix86_str,TAB_POS);
	DisP.insn_flags = _this->flags64;
	(this->*mtd)(ix86_str,DisP);
    }
    else if(DisP.XOP_m==0x09 &&_this->method64) {
	ix86_method mtd = _this->method64;
	TabSpace(ix86_str,TAB_POS);
	DisP.insn_flags = _this->pro_clone;
	(this->*mtd)(ix86_str,DisP);
    }
 }
 else {
 if(x86_Bitness == Bin_Format::Use64)
 {
   if(REX_W(DisP.REX)) DisP.mode|=MOD_WIDE_DATA; /* 66h prefix is ignored if REX prefix is present*/
   if(ix86_table[code].flags64 & K64_DEF32)
   {
     if(REX_W(DisP.REX)) strcpy(ix86_str,ix86_table[code].name64);
     else
     if(!(DisP.mode&MOD_WIDE_DATA))	strcpy(ix86_str,ix86_table[code].name16);
     else				strcpy(ix86_str,ix86_table[code].name32);
   }
   else
   if((DisP.mode&MOD_WIDE_DATA) || (ix86_table[code].flags64 & K64_NOCOMPAT))
		strcpy(ix86_str,ix86_table[code].name64);
   else		strcpy(ix86_str,ix86_table[code].name32);
 }
 else
   strcpy(ix86_str,(DisP.mode&MOD_WIDE_DATA) ? ix86_table[code].name32 : ix86_table[code].name16);
 if(x86_Bitness == Bin_Format::Use64)
 {
   if(ix86_table[code].method64)
   {
	ix86_method mtd = ix86_table[code].method64;
	DisP.insn_flags = ix86_table[code].flags64;
	TabSpace(ix86_str,TAB_POS);
	(this->*mtd)(ix86_str,DisP);
   }
 }
 else
 if(ix86_table[code].method)
 {
	ix86_method mtd = ix86_table[code].method;
	DisP.insn_flags = ix86_table[code].pro_clone;
	TabSpace(ix86_str,TAB_POS);
	(this->*mtd)(ix86_str,DisP);
 }
 /** Special case for jmp call ret modify table name */
 switch(code)
 {
   /** retx, jmpx case */
     case 0xC2:
     case 0xC3:
     case 0xCA:
     case 0xCB:
     case 0xE9:
     case 0xEA:
		if(!ud && !ua) ix86_str[4] = ix86_str[5] = ' ';
		break;
   /** popax, popfx case */
     case 0x61:
     case 0x9D:
		if(!ud && !ua && x86_Bitness < Bin_Format::Use64) ix86_str[4] = 0;
		break;
   /** callx case */
     case 0xE8:
     case 0x9A:
		if(!ud && !ua) ix86_str[5] = ix86_str[6] = ' ';
		break;
   /** pushax, pushfx case */
     case 0x60:
     case 0x9C:
		if(!ud && !ua && x86_Bitness < Bin_Format::Use64) ix86_str[5] = 0;
		break;
     default:   break;
 }
 } /* ifelse XOP */
 ExitDisAsm:
 if(up) DisP.codelen+=up;
 /* control jumps here after SSE2 opcodes */
 if(ix86_segpref[0])
 {
    strcat(ix86_da_out,"seg ");
    ix86_segpref[2] = ';';
    strcat(ix86_da_out,ix86_segpref);
    strcat(ix86_da_out," ");
 }
 if(ix86_da_out[0]) TabSpace(ix86_da_out,TAB_POS);
 strncat(ix86_da_out,ix86_str,MAX_DISASM_OUTPUT);
 Ret.str = ix86_da_out;
 if(x86_Bitness < Bin_Format::Use64)
 if((DisP.pfx&PFX_66) || (DisP.pfx&PFX_67) || x86_Bitness == Bin_Format::Use32)
 {
    if((DisP.pro_clone & IX86_CPUMASK) < IX86_CPU386)
    {
	DisP.pro_clone &= ~IX86_CPUMASK;
	DisP.pro_clone |= IX86_CPU386;
    }
 }
 if((DisP.insn_flags & INSN_VEXW_AS_SIZE) && !REX_w(DisP.REX)) { /* VEX complements REX.W */
	char* last;
	last=&Ret.str[0];
	while(!isspace(*last)) { last++; if(!*last) break; }
	last--;
	switch(*last) {
		case 's': *last='d'; break;
		case 'd': *last='q'; break;
		default: break;
	}
 }
 Ret.pro_clone = DisP.insn_flags;
 Ret.codelen = DisP.codelen;
 Bye:
 return Ret;
}

static const char * CPUNames[] =
{
  " 8086 ",
  "80186 ",
  "80286 ",
  "80386 ",
  "80486 ",
  "80586 ",
  "80686/PII ",
  "PIII   ",
  "P4   ",
  "Prescott "
};

static const char * FPUNames[] =
{
  " 8087 ",
  "      ",
  "80287 ",
  "80387 ",
  "80487 ",
  "      ",
  "80687     ",
  "PIII   ",
  "P4   ",
  "Prescott "
};

static const char * MMXNames[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "P1MMX ",
  "K6-2 3dNow",
  "PIII/K7",
  "P4   ",
  "Prescott "
};

static const char * CPU64Names[] =
{
  " K86-64 CPU ",
  " K86-64 FPU/MMX ",
  " K86-64 SSE/AVX ",
  "       ",
  "       ",
  "       ",
  "       ",
  "      ",
  "      ",
  "      "
};
static const char * altPipesNames[] =
{
  " CPU ALU ",
  " FPU/MMX ALU ",
  " SSE/AVX ALU ",
  "       ",
  "       ",
  "       ",
  "       ",
  "      ",
  "      ",
  "      "
};

static int color_mode=0;

ColorAttr ix86_Disassembler::get_alt_insn_color(unsigned long clone)
{
	color_mode=1;
	if(clone & INSN_SSE) return disasm_cset.engine[2].engine;
	else
	if(clone & (INSN_FPU|INSN_MMX)) return disasm_cset.engine[1].engine;
	else
	return disasm_cset.engine[0].engine;
}

ColorAttr ix86_Disassembler::get_alt_opcode_color(unsigned long clone)
{
  return ((clone & INSN_CPL0) == INSN_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
}

ColorAttr ix86_Disassembler::get_insn_color(unsigned long clone)
{
    color_mode=0;
     if(x86_Bitness == Bin_Format::Use64) return get_alt_insn_color(clone);
     else
     if((clone&INSN_MMX)|(clone&INSN_SSE)) return disasm_cset.cpu_cset[2].clone[clone & IX86_CPUMASK];
     else
       if((clone&INSN_FPU))	return disasm_cset.cpu_cset[1].clone[clone & IX86_CPUMASK];
       else			return disasm_cset.cpu_cset[0].clone[clone & IX86_CPUMASK];
}

ColorAttr ix86_Disassembler::get_opcode_color(unsigned long clone)
{
  return ((clone & INSN_CPL0) == INSN_CPL0)?disasm_cset.opcodes0:disasm_cset.opcodes;
}

const char* ix86_Disassembler::prompt(unsigned idx) const {
    switch(idx) {
	case 0: return "x86Hlp"; break;
	case 2: return "Bitnes"; break;
	default: break;
    }
    return "";
}

bool ix86_Disassembler::action_F1()
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(20000);
	bhelp.close();
    }
    return false;
}

void ix86_Disassembler::show_short_help() const
{
    const char *title;
    std::vector<std::string> strs;
    unsigned evt;
    size_t i,sz;
    TWindow* hwnd;
    Beye_Help bhelp(bctx);

    if(!bhelp.open(true)) return;
    objects_container<char> msgAsmText = bhelp.load_item(20041);
    if(!msgAsmText.empty()) goto ix86hlp_bye;
    strs = bhelp.point_strings(msgAsmText);
    title = msgAsmText.tdata();

    hwnd = CrtHlpWndnls(title,73,22);
    sz=strs.size();
    for(i = 0;i < sz;i++) bhelp.fill_buffer(*hwnd,1,i+1,strs[i]);

    hwnd->goto_xy(5,3);
    if(x86_Bitness == Bin_Format::Use64 || color_mode==1) {
	hwnd->goto_xy(5,3);
	hwnd->set_color(disasm_cset.engine[0].engine);
	hwnd->puts((x86_Bitness == Bin_Format::Use64)?CPU64Names[0]:altPipesNames[0]);
	hwnd->clreol();
	hwnd->goto_xy(5,4);
	hwnd->set_color(disasm_cset.engine[1].engine);
	hwnd->puts((x86_Bitness == Bin_Format::Use64)?CPU64Names[1]:altPipesNames[1]);
	hwnd->clreol();
	hwnd->goto_xy(5,5);
	hwnd->set_color(disasm_cset.engine[2].engine);
	hwnd->puts((x86_Bitness == Bin_Format::Use64)?CPU64Names[2]:altPipesNames[2]);
	hwnd->clreol();
    } else {
	for(i = 0;i < 10;i++) {
	    hwnd->set_color(disasm_cset.cpu_cset[0].clone[i]);
	    hwnd->puts(CPUNames[i]);
	}
	hwnd->goto_xy(5,4);
	for(i = 0;i < 10;i++) {
	    hwnd->set_color(disasm_cset.cpu_cset[1].clone[i]);
	    hwnd->puts(FPUNames[i]);
	}
	hwnd->goto_xy(5,5);
	for(i = 0;i < 10;i++) {
	    hwnd->set_color(disasm_cset.cpu_cset[2].clone[i]);
	    hwnd->puts(MMXNames[i]);
	}
    }
    do {
	evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
    }while(!(evt == KE_ESCAPE || evt == KE_F(10)));
    delete hwnd;
ix86hlp_bye:
    bhelp.close();
}

static const char *use_names[] =
{
   "Use1~6",
   "Use~32",
   "Use6~4",
   "~Auto"
};

bool ix86_Disassembler::action_F3()
{
    unsigned nModes;
    int i;
    nModes = sizeof(use_names)/sizeof(char *);
    if(BITNESS == Bin_Format::Auto) BITNESS = Bin_Format::Use128;
    ListBox lb(bctx);
    i = lb.run(use_names,nModes," Select bitness mode: ",ListBox::Selective|ListBox::UseAcc,BITNESS);
    if(i != -1) {
	if(i == 3) i = Bin_Format::Auto;
	BITNESS = x86_Bitness = Bin_Format::bitness(i);
	return true;
    } else if(BITNESS == 3) BITNESS = x86_Bitness = Bin_Format::Auto;
    return false;
}

int ix86_Disassembler::max_insn_len() const { return MAX_IX86_INSN_LEN; }
Bin_Format::bitness ix86_Disassembler::get_bitness() const { return BITNESS; }
char ix86_Disassembler::clone_short_name(unsigned long clone) { if(x86_Bitness == Bin_Format::Use64) return 'a'; else return ix86CloneSNames[((clone&IX86_CLONEMASK)>>8)&0x07]; }

/*
  x86 disassemblers
  The run_command is expected to be used in sprintf to generate the actual command line.
  The argument is a single string containing the home directory. The temporary files
  created are:
    tmp0: input file
    tmp1: output file
    tmp2: output (stdout) / error (stderr) messages (note the "-s" option)
  The detect_command is any command which prints something to stdout as long as the assembler
  is available.
  Yasm is preferred over Nasm only because at the time of writing only Yasm has 64-bit
  support in its stable release.
  The last element must be a structure containing only NULL pointers.
*/
const assembler_t ix86_Disassembler::assemblers[] = {
  {
    "yasm -f bin -s -o %stmp1 %stmp0 >%stmp2",
#if defined(__MSDOS__) || defined(__OS2__) || defined(__WIN32__)
    "yasm -s --version 2>NUL",
#else
    "yasm -s --version 2>/dev/null",
#endif
  },
  {
    "nasm -f bin -s -o %stmp1 %stmp0 >%stmp2",
#if defined(__MSDOS__) || defined(__OS2__) || defined(__WIN32__)
    "nasm -s -version 2>NUL",
#else
    "nasm -s -version 2>/dev/null",
#endif
  },
  {
    NULL,
    NULL,
  }
};

#ifndef HAVE_PCLOSE
#define pclose(fp) fclose(fp)
#endif

ix86_Disassembler::ix86_Disassembler(BeyeContext& bc,const Bin_Format& b,binary_stream& h,DisMode& _parent )
		    :Disassembler(bc,b,h,_parent)
		    ,bctx(bc)
		    ,parent(_parent)
		    ,main_handle(h)
		    ,bin_format(b)
		    ,x86_Bitness(Bin_Format::Auto)
		    ,active_assembler(-1)
{
  ix86_str = new char [MAX_DISASM_OUTPUT];
  ix86_da_out  = new char [MAX_DISASM_OUTPUT];
  ix86_Katmai_buff = new char [MAX_DISASM_OUTPUT];
  ix86_appstr = new char [MAX_DISASM_OUTPUT];
  ix86_dtile = new char [MAX_DISASM_OUTPUT];
  ix86_appbuffer = new char [MAX_DISASM_OUTPUT];
  ix86_apistr = new char [MAX_DISASM_OUTPUT];
  ix86_modrm_ret = new char [MAX_DISASM_OUTPUT];
#ifdef HAVE_POPEN
  //Assembler initialization
  //Look for an available assembler
  if (active_assembler == -1 && bctx.iniUseExtProgs==true) //Execute this only once
  {
    int i;
    for (i = 0; assemblers[i].detect_command; i++)
    {
      //Assume that the assembler is available if the detect_command prints something to stdout.
      //Note: using the return value of "system()" does not work, at least here.
      FILE *fp;
      fp = popen(assemblers[i].detect_command, "r");
      if (fp == NULL) continue;
      if (fgetc(fp) != EOF) {
	pclose(fp);
	active_assembler=i;
	break;
      }
      pclose(fp);
    }
  }
#endif
}

ix86_Disassembler::~ix86_Disassembler()
{
   delete ix86_str;
   delete ix86_da_out;
   delete ix86_Katmai_buff;
   delete ix86_appstr;
   delete ix86_dtile;
   delete ix86_appbuffer;
   delete ix86_apistr;
   delete ix86_modrm_ret;
}

void ix86_Disassembler::read_ini( Ini_Profile& ini )
{
  std::string tmps;
  unsigned tmpv;
  if(bctx.is_valid_ini_args()) {
    tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode3","1");
    std::istringstream is(tmps);
    is>>tmpv;
    BITNESS= Bin_Format::bitness(tmpv);
    if(BITNESS > 2 && BITNESS != Bin_Format::Auto) BITNESS = Bin_Format::Use16;
    x86_Bitness = BITNESS;
  }
}

void ix86_Disassembler::save_ini( Ini_Profile& ini )
{
    std::ostringstream os;
    os<<BITNESS;
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode3",os.str());
}

const unsigned ix86_Disassembler::CODEBUFFER_LEN=64;

/*
  Assemble "code".
  On success, it returns:
    .insn       = assembled bytes
    .insn_len   = length of assembled code
    .error_code = 0
  On error, it returns:
    .insn       = an NULL-terminated error message
    .insn_len   = undefined
    .error_code = non-zero
*/
AsmRet ix86_Disassembler::assembler(const std::string& code)
{
  char codebuffer[CODEBUFFER_LEN];
  AsmRet result;
  std::ofstream asmf;
  std::ifstream bin, err;
  int c;
  unsigned i;
  char commandbuffer[FILENAME_MAX+1];
  std::string home;

  //Check assembler availability
  memset(&result,0,sizeof(AsmRet));
  if (active_assembler<0) goto noassemblererror;
  if (!assemblers[active_assembler].run_command) goto noassemblererror;

  home = bctx.system().get_home_dir("beye");

  //File cleanup
  sprintf(commandbuffer, "%stmp0", home.c_str());
  for (i=0; i<3; i++)
  {
    remove(commandbuffer);
    commandbuffer[strlen(commandbuffer)-1]++;
  }

  //Generate NASM input file
  sprintf(commandbuffer, "%stmp0", home.c_str());
  asmf.open(commandbuffer,std::ios_base::out);
  if (!asmf.is_open()) goto tmperror;
  if (get_bitness() == Bin_Format::Use16)	asmf<<"BITS 16";
  else if (get_bitness() == Bin_Format::Use64)	asmf<<"BITS 64";
  else					asmf<<"BITS 32";
  asmf<<std::endl;
  asmf<<code;
  asmf.close();

  //Build command line
  i=sprintf(commandbuffer, assemblers[active_assembler].run_command, home.c_str(), home.c_str(), home.c_str());
  if ((i >= FILENAME_MAX) || (int(i) < 0)) goto commandtoolongerror;

  //Run external assembler
  //It happens way too often that system() fails with EAGAIN with
  //no apparent reason. So, don't give up immediately
  i=0;
  do {
    errno=0;
    system(commandbuffer);
    i++;
  }
  while ((errno == EAGAIN) && (i < 10));

  //Read result
  sprintf(commandbuffer, "%stmp1", home.c_str());
  bin.open(commandbuffer,std::ios_base::in);
  if (!bin) goto asmerror;
  i=0;
  while (bin.good() && (i<CODEBUFFER_LEN)) {
    c=bin.get();
    codebuffer[i++]=(char)c;
  }
  bin.close();

  if (i==CODEBUFFER_LEN) goto codetoolongerror;
  if (i==0) goto asmerror;

  result.err_code=0;
  result.insn_len=i;
  result.insn=(MBuffer)codebuffer;
  goto done;

asmerror:
  //Read error message
  sprintf(commandbuffer, "%stmp2", home.c_str());
  err.open(commandbuffer,std::ios_base::in);
  if (!err.is_open()) goto tmperror;
  i=0;
  while (err.good() && (i<CODEBUFFER_LEN-1))
  {
    c = err.get();
    //Only get the 1st error line (usually the most informative)
    if ((((char)c) == '\r') || (((char)c) == '\n')) break;

    codebuffer[i++]=(char)c;
    /*
    Most error messages are in the form:
      [something]: [error description]
    Discarding everything before ':' we obtain the [error description] alone
    */
    if ((char)c == ':')
    {
      i=0;
    }
  }
  err.close();
  codebuffer[i]='\0';
  result.insn = (MBuffer)codebuffer;
  while (result.insn[0] == ' ') //Drop leading spaces
    result.insn++;
  result.err_code=1;
  goto doneerror;

codetoolongerror:
  result.insn=(MBuffer)"Internal error (assembly code too long)";
  result.err_code=2;
  goto doneerror;

tmperror:
  result.insn=(MBuffer)"Can't write temporary files";
  result.err_code=3;
  goto doneerror;

noassemblererror:
  result.insn=(MBuffer)"No assembler available/usable or disabled external programs";
  result.err_code=4;
  goto doneerror;

commandtoolongerror:
  result.insn=(MBuffer)"Internal error (command too long)";
  result.err_code=5;
  goto doneerror;

doneerror:
done:
  //Final cleanup
  sprintf(commandbuffer, "%stmp0", home.c_str());
  for (i=0; i<3; i++)
  {
    remove(commandbuffer);
    commandbuffer[strlen(commandbuffer)-1]++;
  }
  return result;
}

static Disassembler* query_interface(BeyeContext& bc,const Bin_Format& b,binary_stream& h,DisMode& parent) { return new(zeromem) ix86_Disassembler(bc,b,h,parent); }

extern const Disassembler_Info ix86_disassembler_info = {
    DISASM_CPU_IX86,
    "Intel ~ix86 / x86_64",	/**< plugin name */
    query_interface
};
} // namespace	usr

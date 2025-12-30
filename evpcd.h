#pragma once
//[[ DELCARATION ONLY HEADER ]]
#include <stdint.h>

#define EVPCD_EVPCD_EVPCD_EVPCD

#define EVPCD_NULL 0

#define EVPCD_REG_RPC (250)
#define EVPCD_REG_RSP (251)
#define EVPCD_REG_HOSTCALL (253)

#define EVPCD_INS_NOP (0)
#define EVPCD_INS_HOSTCALL (1)

//[[ SECRET! ]]
/*
#define EVPCD_EVPCD_EVPCD_EVPCD
_THIS_IS_A_LONG_MACRO_THAT_IS_LITERALLY_POINTLESS_DONT_USE
_THIS_UNLESS_YOU_FOR_SOME_REASON_WANT_TO_HAVE_A_HUGE_LINE_JUST_TO_REPLACE_ZERO
0

i commented it to avoid issues but final release will have this as a joke, maybe.
*/


extern uint8_t evpcd_f_hostcalled;

int evpcd_err();

int evpcd_init(uint8_t* MemPtr, uint32_t MemSize, uint8_t safe);
int evpcd_step();
int evpcd_start();
int evpcd_end();

extern uint8_t* evpcd_mem_memptr;
extern uint32_t evpcd_mem_memsize;

extern uint8_t evpcd_mem_f_invalidaddress;
extern uint8_t evpcd_mem_f_safe;

uint8_t evpcd_mem_setmem(uint8_t* MemPtr, uint32_t MemSize, uint8_t safe);
uint8_t evpcd_mem_read8(uint32_t adr);
uint16_t evpcd_mem_read16(uint32_t adr);
uint32_t evpcd_mem_read32(uint32_t adr);

uint8_t evpcd_mem_write8(uint32_t adr, uint8_t val);
uint8_t evpcd_mem_write16(uint32_t adr, uint16_t val);
uint8_t evpcd_mem_write32(uint32_t adr, uint32_t val);

uint8_t evpcd_mem_pop8();
uint16_t evpcd_mem_pop16();
uint32_t evpcd_mem_pop32();


uint8_t evpcd_mem_push8(uint8_t val);
uint8_t evpcd_mem_push16(uint16_t val);
uint8_t evpcd_mem_push32(uint32_t val);

uint8_t evpcd_mem_fetch8();
uint16_t evpcd_mem_fetch16();
uint32_t evpcd_mem_fetch32();

extern uint32_t evpcd_cpu_register[256];
extern uint8_t evpcd_cpu_f_condition_met;

uint32_t evpcd_cpu_rreg(uint8_t reg);
void evpcd_cpu_wreg(uint8_t reg, uint32_t val);

extern uint8_t evpcd_isa_f_invalidopcode;
extern uint8_t evpcd_isa_f_safe;

void evpcd_isa_nop();
void evpcd_isa_hostcall();

void evpcd_isa_mov();
void evpcd_isa_xchg();

void evpcd_isa_not();
void evpcd_isa_or();
void evpcd_isa_xor();
void evpcd_isa_and();
void evpcd_isa_shftl();
void evpcd_isa_shftlr();
void evpcd_isa_shftar();

void evpcd_isa_btset();
void evpcd_isa_btclr();
void evpcd_isa_bttest();

void evpcd_isa_ldb();
void evpcd_isa_ldh();
void evpcd_isa_ldw();

void evpcd_isa_stb();
void evpcd_isa_sth();
void evpcd_isa_stw();

void evpcd_isa_cmp();
void evpcd_isa_jmp();
void evpcd_isa_jmpct();
void evpcd_isa_jmpcf();

void evpcd_isa_add();
void evpcd_isa_sub();
void evpcd_isa_mul();
void evpcd_isa_udiv();
void evpcd_isa_sdiv();
void evpcd_isa_urem();
void evpcd_isa_srem();
void evpcd_isa_inc();
void evpcd_isa_dec();

void evpcd_isa_pushb();
void evpcd_isa_pushh();
void evpcd_isa_pushw();

void evpcd_isa_popb();
void evpcd_isa_poph();
void evpcd_isa_popw();

void evpcd_isa_call();
void evpcd_isa_ret();



void evpcd_isa_movi();

void evpcd_isa_ori();
void evpcd_isa_xori();
void evpcd_isa_andi();
void evpcd_isa_shftli();
void evpcd_isa_shftlri();
void evpcd_isa_shftari();

void evpcd_isa_btseti();
void evpcd_isa_btclri();
void evpcd_isa_bttesti();

void evpcd_isa_cmpi();
void evpcd_isa_jmpi();
void evpcd_isa_jmpcti();
void evpcd_isa_jmpcfi();

void evpcd_isa_addi();
void evpcd_isa_subi();
void evpcd_isa_muli();
void evpcd_isa_udivi();
void evpcd_isa_sdivi();
void evpcd_isa_uremi();
void evpcd_isa_sremi();

void evpcd_isa_pushbi();
void evpcd_isa_pushhi();
void evpcd_isa_pushwi();


void evpcd_isa_calli();

void evpcd_isa_init(uint8_t safe);
uint8_t evpcd_isa_exec();



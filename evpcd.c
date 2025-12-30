// ==== CHANGES ====
//[[ No C++ wrapper. ]]
//[[ Fixed macro issues. ]]
//[[ More organized. ]]


//[[ INCLUDES ]]
#include <stdint.h>
#include "evpcd.h"


//[[ MACROS ]]

#define EVPCD_NULL 0

#define EVPCD_REG_RPC (250)
#define EVPCD_REG_RSP (251)
#define EVPCD_REG_RET (252)
#define EVPCD_REG_HOSTCALL (253)

#define EVPCD_INS_NOP (0)
#define EVPCD_INS_HOSTCALL (1)

//[[ DECLARATIONS ]]
uint8_t evpcd_mem_setmem(uint8_t* MemPtr, uint32_t MemSize, uint8_t safe);
extern uint8_t* evpcd_mem_memptr;
extern uint32_t evpcd_mem_memsize;

extern uint32_t evpcd_cpu_register[256];
uint32_t evpcd_cpu_rreg(uint8_t reg);
void evpcd_cpu_wreg(uint8_t reg, uint32_t val);

extern uint8_t evpcd_isa_f_invalidopcode;
extern uint8_t evpcd_mem_f_invalidaddress;

void evpcd_isa_init(uint8_t safe);
extern uint8_t evpcd_isa_exec();

//[[ PRIMARY ]]


uint8_t evpcd_f_hostcalled = 0;

int evpcd_err(){
    if(evpcd_isa_f_invalidopcode){
        return evpcd_isa_f_invalidopcode;
    }
    if(evpcd_mem_f_invalidaddress){
        return evpcd_mem_f_invalidaddress + 256; //so not potential opcode
    }
    return 0;
}
int evpcd_init(uint8_t* MemPtr, uint32_t MemSize, uint8_t safe){
    //MEM
    uint8_t code = evpcd_mem_setmem(MemPtr, MemSize, safe);
    evpcd_isa_init(safe);
    if(code){
        return code;
    }
    //CPU
    evpcd_cpu_wreg(EVPCD_REG_RPC, 0);
    evpcd_cpu_wreg(EVPCD_REG_RSP, 0);
    return 0;
}
int evpcd_step(){
    return evpcd_isa_exec();
}
int evpcd_start(){
    while(1){
        uint8_t rt = evpcd_step();

        //[[ HOST-CALL CHECK ]]
        if(evpcd_f_hostcalled){ return 0;}

        //[[ ERROR CHECK ]]
        int err = evpcd_err();
        if(err){return err;}

    }
    return 0;
}
int evpcd_end(){

    evpcd_mem_memptr = EVPCD_NULL;
    evpcd_mem_memsize = 0;

    return 0;
}



// ==== EVPCD MEMORY ====

//VAR
uint8_t* evpcd_mem_memptr = EVPCD_NULL;
uint32_t evpcd_mem_memsize = 0;

uint8_t evpcd_mem_f_invalidaddress = 0;

uint8_t evpcd_mem_f_safe = 0;

//FN
uint8_t evpcd_mem_setmem(uint8_t* MemPtr, uint32_t MemSize, uint8_t safe){
    if(!safe){
        goto skip;
    }

    if(MemSize == 0){
        return 1; //error
    }
    if(MemPtr == EVPCD_NULL){
        return 2;
    }




    skip:
    evpcd_mem_memptr = MemPtr;
    evpcd_mem_memsize = MemSize;
    evpcd_mem_f_safe = safe;

    return 0;
}
uint8_t evpcd_mem_read8(uint32_t adr){
    if(adr >= evpcd_mem_memsize && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 1;
        return 0;
    }
    return evpcd_mem_memptr[adr];
}
uint16_t evpcd_mem_read16(uint32_t adr){
    if(adr + 1 >= evpcd_mem_memsize && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 2;
        return 0;
    }
    //[[ This is big endian, remember! ]]
    return ((evpcd_mem_memptr[adr] << 8) | evpcd_mem_memptr[adr+1]);    
}
uint32_t evpcd_mem_read32(uint32_t adr){
    if(adr + 3 >= evpcd_mem_memsize && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 3;
        return 0;
    }
    //[[ This is big endian, remember! ]]
    return ((evpcd_mem_memptr[adr] << 24) | (evpcd_mem_memptr[adr+1] << 16) | (evpcd_mem_memptr[adr+2] << 8) | (evpcd_mem_memptr[adr+3]));    
}

uint8_t evpcd_mem_write8(uint32_t adr, uint8_t val){
    if(adr >= evpcd_mem_memsize && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 4;
        return 1;
    }

    evpcd_mem_memptr[adr] = val;

    return 0;
}
uint8_t evpcd_mem_write16(uint32_t adr, uint16_t val){
    if(adr + 1 >= evpcd_mem_memsize && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 5;
        return 1;
    }

    evpcd_mem_memptr[adr] = val >> 8; //MSB!
    evpcd_mem_memptr[adr + 1] = val & 0xFF;

    return 0;
}
uint8_t evpcd_mem_write32(uint32_t adr, uint32_t val){
    if(adr + 3 >= evpcd_mem_memsize && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 6;
        return 1;
    }

    evpcd_mem_memptr[adr] = val >> 24; //MSB!
    evpcd_mem_memptr[adr + 1] = (val >> 16) & 0xFF; //The & 0xff part is for extra saftey
    evpcd_mem_memptr[adr + 2] = (val >> 8) & 0xFF;
    evpcd_mem_memptr[adr + 3] = (val) & 0xFF;


    return 0;
}

uint8_t evpcd_mem_pop8(){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RSP);
    //Get address from stack pointer [ current ]
    uint8_t val = evpcd_mem_read8(adr);


    evpcd_cpu_wreg(EVPCD_REG_RSP, adr-1); //[ write new address after subtracting]
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 7; //update error!
        return 0;
    }

    //[[ NOTE : ]]
    //Because this takes the current address, the push op should instead increment
    //the pointer THEN actually push the value (write)


    return val;
}
uint16_t evpcd_mem_pop16(){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RSP);
    uint16_t val = (evpcd_mem_read8(adr) | (evpcd_mem_read8(adr - 1) << 8));
    //starts with LSB since we're going backwards


    evpcd_cpu_wreg(EVPCD_REG_RSP, adr-2);
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 8; //update error!
        return 0;
    }


    return val;
}
uint32_t evpcd_mem_pop32(){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RSP);
    uint32_t val = (evpcd_mem_read8(adr) | (evpcd_mem_read8(adr-1) << 8) | (evpcd_mem_read8(adr-2) << 16) | (evpcd_mem_read8(adr-3) << 24));
    //starts with LSB since we're going backwards


    evpcd_cpu_wreg(EVPCD_REG_RSP, adr-4);
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 9; //update error!
        return 0;
    }


    return val;
}

uint8_t evpcd_mem_push8(uint8_t val){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RSP);
    //Write value
    evpcd_mem_write8(adr+1, val);
    //update ptr
    evpcd_cpu_wreg(EVPCD_REG_RSP, adr+1);
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 10; //error
        return 1; //failed
    }

    return 0; //success
}
uint8_t evpcd_mem_push16(uint16_t val){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RSP);
    //Write value
    evpcd_mem_write16(adr+1, val);
    //update ptr
    evpcd_cpu_wreg(EVPCD_REG_RSP, adr+2); //adding 2 to the pointer for correct offset
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 11; //error
        return 1; //failed
    }

    return 0; //success
}
uint8_t evpcd_mem_push32(uint32_t val){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RSP);
    //Write value
    evpcd_mem_write32(adr+1, val);
    //update ptr
    evpcd_cpu_wreg(EVPCD_REG_RSP, adr+4); //adding 4 to the pointer for correct offset
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 12; //error
        return 1; //failed
    }

    return 0; //success
}

uint8_t evpcd_mem_fetch8(){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RPC); //From PC instead of SP
    evpcd_cpu_wreg(EVPCD_REG_RPC, adr+1); //increment by one byte the ptr
    uint8_t val = evpcd_mem_read8(adr);
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 13; //error
        return 0; //failed
    }

    return val;
}
uint16_t evpcd_mem_fetch16(){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RPC); //From PC instead of SP
    evpcd_cpu_wreg(EVPCD_REG_RPC, adr+2); //increment by 2
    uint16_t val = evpcd_mem_read16(adr);
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 14; //error
        return 0; //failed
    }

    return val;
}
uint32_t evpcd_mem_fetch32(){
    uint32_t adr = evpcd_cpu_rreg(EVPCD_REG_RPC); //From PC instead of SP
    evpcd_cpu_wreg(EVPCD_REG_RPC, adr+4); //increment by 4
    uint32_t val = evpcd_mem_read32(adr);
    if(evpcd_mem_f_invalidaddress && evpcd_mem_f_safe){
        evpcd_mem_f_invalidaddress = 15; //error
        return 0; //failed
    }

    return val;
}
// ==== EVPCD CPU ====
uint32_t evpcd_cpu_register[256] = {0};

uint8_t evpcd_cpu_f_condition_met = 0;

uint32_t evpcd_cpu_rreg(uint8_t reg){
    return evpcd_cpu_register[reg];
}
void evpcd_cpu_wreg(uint8_t reg, uint32_t val){
    evpcd_cpu_register[reg] = val;
}

// ==== EVPCD ISA ====

uint8_t evpcd_isa_f_invalidopcode = 0;
uint8_t evpcd_isa_f_safe = 0;


// [[ INSTRUCTS ]]
void evpcd_isa_nop(){
    //Do nothing;
}
void evpcd_isa_hostcall(){
    evpcd_f_hostcalled = 1;
}

void evpcd_isa_mov(){
    //r1 <- (=) r2; 8[r1]8[r2]
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    evpcd_cpu_wreg(reg1, evpcd_cpu_rreg(reg2));
}
void evpcd_isa_xchg(){
    //exchange
    //r1 = r2, r2 = r1
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t tmp = evpcd_cpu_rreg(reg1);

    evpcd_cpu_wreg(reg1, evpcd_cpu_rreg(reg2));
    evpcd_cpu_wreg(reg2, tmp);
}

void evpcd_isa_not(){
    //r1 <- !r2
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val = evpcd_cpu_rreg(reg2);
    val = ~val;

    evpcd_cpu_wreg(reg1, val);
}
void evpcd_isa_or(){
    //r1 = r2 or r3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    uint32_t val1 = val2 | val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_xor(){
    //r1 = r2 xor r3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = val2 ^ val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_and(){
    //r1 = r2 or r3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = val2 & val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_shftl(){
    //logical left shift
    //r1 = r2 << r3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = val2 << val3;

    evpcd_cpu_wreg(reg1, val1);    
}
void evpcd_isa_shftlr(){
    //logical right shift
    //r1 = r2 >> r3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = val2 >> val3;

    evpcd_cpu_wreg(reg1, val1);    
}
void evpcd_isa_shftar(){
    //[[ WARNING, behaviour not guaranteed, avoid using this. ]]
    //arithmitic right shift
    //r1 = r2 arth>> r3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    int32_t val1 = (int32_t)val2 >> (int32_t)val3;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);    
}


void evpcd_isa_btset(){
    //r1 <- (r2[r3] = 1)
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = val2 | (1u << (val3));

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_btclr(){
    //r1 <- (r2[r3] = 0)
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = val2 & ~(1u << (val3));

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_bttest(){
    //condition met flag/reg1 = ?: reg2[reg3] == 1?
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);
    
    uint32_t val1 = (((val2) & (1u << (val3))) != 0);
    if(val1){
        evpcd_cpu_f_condition_met = 1;
        val1 = 1;
    }else{
        evpcd_cpu_f_condition_met = 0;
        val1 = 0;
    }

    evpcd_cpu_wreg(reg1, val1);
}


void evpcd_isa_ldb(){
    //reg1 = mem[reg2] [0:8]
    //Load byte
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint8_t val1 = evpcd_mem_read8(val2);

    evpcd_cpu_wreg(reg1, val1);
    
}
void evpcd_isa_ldh(){
    //reg1 = mem[reg2] [0:16]
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint16_t val1 = evpcd_mem_read16(val2);

    evpcd_cpu_wreg(reg1, val1);
    
}
void evpcd_isa_ldw(){
    //reg1 = mem[reg2] [0:32]
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = evpcd_mem_read32(val2);

    evpcd_cpu_wreg(reg1, val1);
}

void evpcd_isa_stb(){
    //mem[reg1] = reg2 [0:8]
    //Store byte
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val1 = evpcd_cpu_rreg(reg1);
    uint32_t val2 = evpcd_cpu_rreg(reg2);

    evpcd_mem_write8(val1, (uint8_t)val2 & 0xFF);
}
void evpcd_isa_sth(){
    //mem[reg1] = reg2 [0:16]
    //Store byte
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val1 = evpcd_cpu_rreg(reg1);
    uint32_t val2 = evpcd_cpu_rreg(reg2);

    evpcd_mem_write16(val1, (uint16_t)val2 & 0xFFFF);
}
void evpcd_isa_stw(){
    //mem[reg1] = reg2 [0:32]
    //Store byte
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val1 = evpcd_cpu_rreg(reg1);
    uint32_t val2 = evpcd_cpu_rreg(reg2);

    evpcd_mem_write32(val1, val2);
}

void evpcd_isa_cmp(){
    //add more specialized cmp instructions later
    //For performance ig... yeah also cmpi with
    //register and immediate
    //[[ For specialized, do much later on, Because they aren't required ]]
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t mode = evpcd_mem_fetch8();

    uint32_t val1 = evpcd_cpu_rreg(reg1);
    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint8_t result = 0;

    switch(mode){
        default: break;
        //[[ copied from evpcd2 line by line ]]
        case 0: if(val1 == 0){result = 1; break;}
        case 1: if(val2 == 0){result = 1; break;}
        case 2: if((int32_t)val1 < 0){result = 1; break;}
        case 3: if((int32_t)val2 < 0){result = 1; break;}
        case 4: if(val1 == 0 && val2 == 0){result = 1; break;}
        case 5: if(val1 == val2){result = 1; break;}

        case 6: if(val1 > val2){result = 1; break;}
        case 7: if(val1 >= val2){result = 1; break;}
        case 8: if((int32_t)val1 > (int32_t)val2){result = 1; break;}
        case 9: if((int32_t)val1 >= (int32_t)val2){result = 1; break;}

        case 10: if(val1 < val2){result = 1; break;}
        case 11: if(val1 <= val2){result = 1; break;}

        case 12: if((int32_t)val1 < (int32_t)val2){result = 1; break;}
        case 13: if((int32_t)val1 <= (int32_t)val2){result = 1; break;}

        case 14: if(val1 != val2){result = 1; break;}

        case 15: if(val1 && val2){result = 1; break;}
        case 16: if(val1 || val2){result = 1; break;}

        case 17: if(val1 != 0){result = 1; break;}
        case 18: if(val2 != 0){result = 1; break;}

        //[[ added segment ]]

        //essentially, here we treat the second arg as an imm, not a register
        case 19: if(val1 == reg2){result = 1; break;}
        case 20: if(val1 != reg2){result = 1; break;}

        case 21: if(val1 > reg2){result = 1; break;}
        case 22: if(val1 >= reg2){result = 1; break;}

        case 23: if((int32_t)val1 > (int32_t)reg2){result = 1; break;}
        case 24: if((int32_t)val1 >= (int32_t)reg2){result = 1; break;}

        case 25: if(val1 < reg2){result = 1; break;}
        case 26: if(val1 <= reg2){result = 1; break;}

        case 27: if((int32_t)val1 < (int32_t)reg2){result = 1; break;}
        case 28: if((int32_t)val1 <= (int32_t)reg2){result = 1; break;}
    }

    evpcd_cpu_f_condition_met = result;
}

void evpcd_isa_jmp(){
    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1);
    evpcd_cpu_wreg(EVPCD_REG_RPC, val1);
}
void evpcd_isa_jmpct(){
    if(evpcd_cpu_f_condition_met){
        evpcd_isa_jmp(); //Just call jump if condition met
    }
    evpcd_mem_fetch8();
}
void evpcd_isa_jmpcf(){
    if(!evpcd_cpu_f_condition_met){
        evpcd_isa_jmp();
    }    
    evpcd_mem_fetch8();
}

void evpcd_isa_add(){
    //reg1 = reg2 + reg3
    //addition
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    uint32_t val1 = val2 + val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_sub(){
    //reg1 = reg2 - reg3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    uint32_t val1 = val2 - val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_mul(){
    //reg1 = reg2 * reg3
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    uint32_t val1 = val2 * val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_udiv(){
    //reg1 = reg2 / reg3
    //unsigned division
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    uint32_t val1 = val2 / val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_sdiv(){
    //reg1 = reg2 / reg3
    //signed division
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    int32_t val1 = (int32_t)val2 / (int32_t)val3;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);
}
void evpcd_isa_urem(){
    //reg1 = reg2 % reg3
    //unsigned remainder
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    uint32_t val1 = val2 % val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_srem(){
    //reg1 = reg2 % reg3
    //signed remainder
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t reg3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    uint32_t val3 = evpcd_cpu_rreg(reg3);

    int32_t val1 = (int32_t)val2 % (int32_t)val3;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);
}
void evpcd_isa_inc(){
    //reg1++;
    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1) + 1;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);
}
void evpcd_isa_dec(){
    //reg1--;
    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1) - 1;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);
}

void evpcd_isa_pushb(){
    //reg1[0:8] -> stack[SP]

    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1);
    evpcd_mem_push8(val1 & 0xff);

}
void evpcd_isa_pushh(){
    //reg1[0:16] -> stack[SP]

    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1);
    evpcd_mem_push16(val1 & 0xffff);

}
void evpcd_isa_pushw(){
    //reg1[0:16] -> stack[SP]

    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1);
    evpcd_mem_push32(val1);

}

void evpcd_isa_popb(){
    //reg1[0:8] <- stack[SP]

    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_mem_pop8();
    evpcd_cpu_wreg(reg1, val1);

}
void evpcd_isa_poph(){
    //reg1[0:16] <- stack[SP]

    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_mem_pop16();
    evpcd_cpu_wreg(reg1, val1);

}
void evpcd_isa_popw(){
    //reg1[0:32] <- stack[SP]

    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_mem_pop32();
    evpcd_cpu_wreg(reg1, val1);

}

void evpcd_isa_call(){
    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t val1 = evpcd_cpu_rreg(reg1);

    uint32_t current_adr = evpcd_cpu_rreg(EVPCD_REG_RPC);
    evpcd_mem_push32(current_adr); //Store by pushing the current address

    //Write address to PC

    evpcd_cpu_wreg(EVPCD_REG_RPC, val1);

}
void evpcd_isa_ret(){
    uint32_t return_adr = evpcd_mem_pop32();

    evpcd_cpu_wreg(EVPCD_REG_RPC, return_adr);
    
}

//imm variant
void evpcd_isa_movi(){
    //r1 <- imm, 8[r1]32[imm]
    uint8_t reg1 = evpcd_mem_fetch8();
    uint32_t imm = evpcd_mem_fetch32();

    evpcd_cpu_wreg(reg1, imm);
}
void evpcd_isa_ori(){
    //r1 = r2 or imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 | val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_xori(){
    //r1 = r2 xor imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 ^ val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_andi(){
    //r1 = r2 and imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 & val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_shftli(){
    //logical left shift
    //r1 = r2 << imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t val3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    
    uint32_t val1 = val2 << val3;

    evpcd_cpu_wreg(reg1, val1);    
}
void evpcd_isa_shftlri(){
    //logical right shift
    //r1 = r2 >> imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t val3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    
    uint32_t val1 = val2 >> val3;

    evpcd_cpu_wreg(reg1, val1);    
}
void evpcd_isa_shftari(){
    //[[ WARNING, behaviour not guaranteed, avoid using this. ]]
    //arithmitic right shift
    //r1 = r2 arth>> imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    uint8_t val3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    
    int32_t val1 = (int32_t)val2 >> (int32_t)val3;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);    
}

void evpcd_isa_btseti(){
    //r1 <- (r2[v3] = 1)
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint8_t val3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    
    uint32_t val1 = val2 | (1u << (val3));

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_btclri(){
    //r1 <- (r2[v3] = 0)
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint8_t val3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    
    uint32_t val1 = val2 & ~(1u << (val3));

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_bttesti(){
    //condition met flag/reg1 = ?: reg2[imm] == 1?
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint8_t val3 = evpcd_mem_fetch8();

    uint32_t val2 = evpcd_cpu_rreg(reg2);
    
    uint32_t val1 = (((val2) & (1u << (val3))) != 0);
    if(val1){
        evpcd_cpu_f_condition_met = 1;
        val1 = 1;
    }else{
        evpcd_cpu_f_condition_met = 0;
        val1 = 0;
    }

    evpcd_cpu_wreg(reg1, val1);
}

void evpcd_isa_cmpi(){

    uint8_t reg1 = evpcd_mem_fetch8();

    uint32_t imm = evpcd_mem_fetch32();

    uint8_t mode = evpcd_mem_fetch8();

    uint32_t val1 = evpcd_cpu_rreg(reg1);
    uint32_t val2 = imm;

    uint8_t result = 0;

    switch(mode){
        default: break;
        //[[ copied from evpcd2 line by line ]]
        case 0: if(val1 == 0){result = 1; break;}
        case 1: if(val2 == 0){result = 1; break;}
        case 2: if((int32_t)val1 < 0){result = 1; break;}
        case 3: if((int32_t)val2 < 0){result = 1; break;}
        case 4: if(val1 == 0 && val2 == 0){result = 1; break;}
        case 5: if(val1 == val2){result = 1; break;}

        case 6: if(val1 > val2){result = 1; break;}
        case 7: if(val1 >= val2){result = 1; break;}
        case 8: if((int32_t)val1 > (int32_t)val2){result = 1; break;}
        case 9: if((int32_t)val1 >= (int32_t)val2){result = 1; break;}

        case 10: if(val1 < val2){result = 1; break;}
        case 11: if(val1 <= val2){result = 1; break;}

        case 12: if((int32_t)val1 < (int32_t)val2){result = 1; break;}
        case 13: if((int32_t)val1 <= (int32_t)val2){result = 1; break;}

        case 14: if(val1 != val2){result = 1; break;}

        case 15: if(val1 && val2){result = 1; break;}
        case 16: if(val1 || val2){result = 1; break;}

        case 17: if(val1 != 0){result = 1; break;}
        case 18: if(val2 != 0){result = 1; break;}

    }

    evpcd_cpu_f_condition_met = result;
}
void evpcd_isa_jmpi(){
    uint32_t val1 = evpcd_mem_fetch32();
    evpcd_cpu_wreg(EVPCD_REG_RPC, val1);
}
void evpcd_isa_jmpcti(){
    if(evpcd_cpu_f_condition_met){
        evpcd_isa_jmpi(); //Just call jump if condition met
    }
    evpcd_mem_fetch32();
}
void evpcd_isa_jmpcfi(){
    if(!evpcd_cpu_f_condition_met){
        evpcd_isa_jmpi();
    }
    evpcd_mem_fetch32();
}

void evpcd_isa_addi(){
    //reg1 = reg2 + imm
    //addition
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();

    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 + val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_subi(){
    //reg1 = reg2 - imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    
    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 - val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_muli(){
    //reg1 = reg2 * imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    
    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 * val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_udivi(){
    //reg1 = reg2 / imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    
    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 / val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_sdivi(){
    //reg1 = reg2 / imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    
    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    int32_t val1 = (int32_t)val2 / (int32_t)val3;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);
}
void evpcd_isa_uremi(){
    //reg1 = reg2 % imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    
    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    uint32_t val1 = val2 % val3;

    evpcd_cpu_wreg(reg1, val1);
}
void evpcd_isa_sremi(){
    //reg1 = reg2 % imm
    uint8_t reg1 = evpcd_mem_fetch8();
    uint8_t reg2 = evpcd_mem_fetch8();
    
    uint32_t val3 = evpcd_mem_fetch32();

    uint32_t val2 = evpcd_cpu_rreg(reg2);

    int32_t val1 = (int32_t)val2 % (int32_t)val3;

    evpcd_cpu_wreg(reg1, (uint32_t)val1);
}

void evpcd_isa_pushbi(){
    //val1[0:8] -> stack[SP]

    uint8_t val1 = evpcd_mem_fetch8();
    evpcd_mem_push8(val1);

}
void evpcd_isa_pushhi(){
    //val1[0:16] -> stack[SP]

    uint16_t val1 = evpcd_mem_fetch16();
    evpcd_mem_push16(val1);

}
void evpcd_isa_pushwi(){
    //val1[0:32] -> stack[SP]

    uint32_t val1 = evpcd_mem_fetch32();
    evpcd_mem_push32(val1);

}

void evpcd_isa_calli(){
    uint32_t val1 = evpcd_mem_fetch32();

    uint32_t current_adr = evpcd_cpu_rreg(EVPCD_REG_RPC);
    evpcd_mem_push32(current_adr); //Store by pushing the current address

    //Write address to PC

    evpcd_cpu_wreg(EVPCD_REG_RPC, val1);

}

// [[ PRIMARY ]]
void evpcd_isa_init(uint8_t safe){
    evpcd_isa_f_safe = safe;

}
uint8_t evpcd_isa_exec(){
    uint8_t opcode = evpcd_mem_fetch8();

    //Switch for function array later or smth
    switch(opcode){
        default: if(evpcd_isa_f_safe){evpcd_isa_f_invalidopcode = opcode;} break;
        case 0: evpcd_isa_nop(); break;
        case 1: evpcd_isa_hostcall(); break;

        case 2: evpcd_isa_mov(); break;
        case 3: evpcd_isa_xchg(); break;
        
        case 4: evpcd_isa_not(); break;
        case 5: evpcd_isa_or(); break;
        case 6: evpcd_isa_xor(); break;
        case 7: evpcd_isa_and(); break;

        case 8: evpcd_isa_shftl(); break;
        case 9: evpcd_isa_shftlr(); break;
        case 10: evpcd_isa_shftar(); break;

        case 11: evpcd_isa_btset(); break;
        case 12: evpcd_isa_btclr(); break;
        case 13: evpcd_isa_bttest(); break;

        case 14: evpcd_isa_ldb(); break;
        case 15: evpcd_isa_ldh(); break;
        case 16: evpcd_isa_ldw(); break;

        case 17: evpcd_isa_stb(); break;
        case 18: evpcd_isa_sth(); break;
        case 19: evpcd_isa_stw(); break;

        case 20: evpcd_isa_cmp(); break;

        case 21: evpcd_isa_jmp(); break;
        case 22: evpcd_isa_jmpct(); break;
        case 23: evpcd_isa_jmpcf(); break;

        case 24: evpcd_isa_add(); break;
        case 25: evpcd_isa_sub(); break;
        case 26: evpcd_isa_mul(); break;
        case 27: evpcd_isa_udiv(); break;
        case 28: evpcd_isa_sdiv(); break;
        case 29: evpcd_isa_urem(); break;
        case 30: evpcd_isa_srem(); break;

        case 31: evpcd_isa_inc(); break;
        case 32: evpcd_isa_dec(); break;

        case 33: evpcd_isa_pushb(); break;
        case 34: evpcd_isa_pushh(); break;
        case 35: evpcd_isa_pushw(); break;

        case 36: evpcd_isa_popb(); break;
        case 37: evpcd_isa_poph(); break;
        case 38: evpcd_isa_popw(); break;

        case 39: evpcd_isa_call(); break;
        case 40: evpcd_isa_ret(); break;

        //[[ IMMEDIATE VARIANTS ]]

        case 128 + 2: evpcd_isa_movi(); break;

        
        case 128 + 5: evpcd_isa_ori(); break;
        case 128 + 6: evpcd_isa_xori(); break;
        case 128 + 7: evpcd_isa_andi(); break;

        case 128 + 8: evpcd_isa_shftli(); break;
        case 128 + 9: evpcd_isa_shftlri(); break;
        case 128 + 10: evpcd_isa_shftari(); break;

        case 128 + 11: evpcd_isa_btseti(); break;
        case 128 + 12: evpcd_isa_btclri(); break;
        case 128 + 13: evpcd_isa_bttesti(); break;


        case 128 + 20: evpcd_isa_cmpi(); break;

        case 128 + 21: evpcd_isa_jmpi(); break;
        case 128 + 22: evpcd_isa_jmpcti(); break;
        case 128 + 23: evpcd_isa_jmpcfi(); break;

        case 128 + 24: evpcd_isa_addi(); break;
        case 128 + 25: evpcd_isa_subi(); break;
        case 128 + 26: evpcd_isa_muli(); break;
        case 128 + 27: evpcd_isa_udivi(); break;
        case 128 + 28: evpcd_isa_sdivi(); break;
        case 128 + 29: evpcd_isa_uremi(); break;
        case 128 + 30: evpcd_isa_sremi(); break;


        case 128 + 33: evpcd_isa_pushbi(); break;
        case 128 + 34: evpcd_isa_pushhi(); break;
        case 128 + 35: evpcd_isa_pushwi(); break;


        case 128 + 39: evpcd_isa_calli(); break;


        
    }

    return opcode;
}






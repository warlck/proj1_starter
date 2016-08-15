#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdint.h>

/* IMPLEMENT ME - see documentation in translate.c */
unsigned write_pass_one(FILE* output, const char* name, char** args, int num_args);

/* IMPLEMENT ME - see documentation in translate.c */
int translate_inst(FILE* output, const char* name, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* symtbl, SymbolTable* reltbl);

/* Declaring helper functions: */

int write_rtype(uint8_t funct, FILE* output, char** args, size_t num_args);

int write_shift(uint8_t funct, FILE* output, char** args, size_t num_args);

/* SOLUTION CODE BELOW */

int write_jr(uint8_t funct, FILE* output, char** args, size_t num_args);

int write_addiu(uint8_t opcode, FILE* output, char** args, size_t num_args);

int write_ori(uint8_t opcode, FILE* output, char** args, size_t num_args);

int write_lui(uint8_t opcode, FILE* output, char** args, size_t num_args);

int write_mem(uint8_t opcode, FILE* output, char** args, size_t num_args);

int write_branch(uint8_t opcode, FILE* output, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* symtbl);

int write_jump(uint8_t opcode, FILE* output, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* reltbl);

int is_valid_register(int val);

int is_valid_signed_immediate(long int imm);

uint32_t make_rtype_instruction(uint8_t funct, int rd, int rs, int rt, int shamt);

uint32_t make_itype_instruction(uint8_t opcode, int rs, int rt, uint16_t imm);

int write_signed_imm(uint8_t opcode, FILE* output, char** args, size_t num_args);

int write_unsigned_imm(uint8_t opcode, FILE* output, char** args, size_t num_args);

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "tables.h"
#include "translate_utils.h"
#include "translate.h"

/* Writes instructions during the assembler's first pass to OUTPUT. The case
   for general instructions has already been completed, but you need to write
   code to translate the li and blt pseudoinstructions. Your pseudoinstruction 
   expansions should not have any side effects.

   NAME is the name of the instruction, ARGS is an array of the arguments, and
   NUM_ARGS specifies the number of items in ARGS.

   Error checking for regular instructions are done in pass two. However, for
   pseudoinstructions, you must make sure that ARGS contains the correct number
   of arguments. You do NOT need to check whether the registers / label are 
   valid, since that will be checked in part two.

   Also for li:
    - make sure that the number is representable by 32 bits. (Hint: the number 
        can be both signed or unsigned).
    - if the immediate can fit in the imm field of an addiu instruction, then
        expand li into a single addiu instruction. Otherwise, expand it into 
        a lui-ori pair.

   And for blt:
    - your expansion should use the fewest number of instructions possible.

   MARS has slightly different translation rules for li, and it allows numbers
   larger than the largest 32 bit number to be loaded with li. You should follow
   the above rules if MARS behaves differently.

   Use fprintf() to write. If writing multiple instructions, make sure that 
   each instruction is on a different line.

   Returns the number of instructions written (so 0 if there were any errors).
 */
unsigned write_pass_one(FILE* output, const char* name, char** args, int num_args) {
    if (strcmp(name, "li") == 0) {
        return write_li(output, args, num_args);
    } else if (strcmp(name, "blt") == 0) {
        /* YOUR CODE HERE */
        return write_blt(output, args, num_args);
    } else {
        write_inst_string(output, name, args, num_args);
        return 1;
    }
}

int write_li(FILE *output, char **args, int num_args)  {
  if (num_args != 2) return 0;
  int32_t min  = INT32_MIN;
  int32_t max =  INT32_MAX;
  int32_t immediate;

  int err_unsigned = translate_num(&immediate, args[1], 0, UINT32_MAX);
  int err_signed = translate_num(&immediate, args[1], min, max);
  if (err_signed == -1 && err_unsigned == -1) return 0;

  if (is_valid_signed_16_bit(immediate)) { 
    char *name = "addiu";
    char *args_temp[3] = {args[0], "$0", args[1]};
    num_args = 3;
    write_inst_string(output, name, args_temp, num_args);
    return 1;
  } 


  // expand to lui and ori
  // prepare the lui args and write it down to output
  char *name = "lui";
  uint32_t imm = immediate & 0xffff0000;
  imm >>= 16;

  char lui_imm_str[32];
  int n  = sprintf(lui_imm_str, "0x%08x", imm);
  if (n < 0) return 0;

  char *args_lui[3] = {"$at", lui_imm_str};
  num_args = 2;

  write_inst_string(output, name, args_lui, num_args);

  // prepare the ori args and write it down to output
  name = "ori";
  imm = immediate & 0x0000ffff;
  char ori_imm_str[32];
  int m = sprintf(ori_imm_str, "0x%08x", imm);
  if (m < 0) return 0;

  char *args_ori[3] = {args[0], "$at", ori_imm_str};
  num_args = 3;
  write_inst_string(output, name, args_ori, num_args);
  return 2;
}



int write_blt(FILE *output, char **args, int num_args) {
  if (num_args!= 3) return 0;
  char *args_slt[3] = {"$at", args[0], args[1]};
  char *args_bne[3] = {"$at", "$0", args[2]};
  write_inst_string(output, "slt", args_slt, num_args);
  write_inst_string(output, "bne", args_bne, num_args);
  return 2;
}


/* Writes the instruction in hexadecimal format to OUTPUT during pass #2.
   
   NAME is the name of the instruction, ARGS is an array of the arguments, and
   NUM_ARGS specifies the number of items in ARGS. 

   The symbol table (SYMTBL) is given for any symbols that need to be resolved
   at this step. If a symbol should be relocated, it should be added to the
   relocation table (RELTBL), and the fields for that symbol should be set to
   all zeros. 

   You must perform error checking on all instructions and make sure that their
   arguments are valid. If an instruction is invalid, you should not write 
   anything to OUTPUT but simply return -1. MARS may be a useful resource for
   this step.

   Note the use of helper functions. Consider writing your own! If the function
   definition comes afterwards, you must declare it first (see translate.h).

   Returns 0 on success and -1 on error. 
 */
int translate_inst(FILE* output, const char* name, char** args, size_t num_args, uint32_t addr,
    SymbolTable* symtbl, SymbolTable* reltbl) {
    if (strcmp(name, "addu") == 0)       return write_rtype (0x21, output, args, num_args);
    else if (strcmp(name, "or") == 0)    return write_rtype (0x25, output, args, num_args);
    else if (strcmp(name, "slt") == 0)   return write_rtype (0x2a, output, args, num_args);
    else if (strcmp(name, "sltu") == 0)  return write_rtype (0x2b, output, args, num_args);
    else if (strcmp(name, "sll") == 0)   return write_shift (0x00, output, args, num_args);
    else if (strcmp(name, "jr") == 0)    return write_jr    (0x08, output, args, num_args);
    else if (strcmp(name, "addiu") == 0) return write_addiu (0x09, output, args, num_args);
    else if (strcmp(name, "ori") == 0)   return write_ori   (0x0d, output, args, num_args);
    else if (strcmp(name, "lui") == 0)   return write_lui   (0x0f, output, args, num_args);
    else if (strcmp(name, "lb") == 0)    return write_mem   (0x20, output, args, num_args);
    else if (strcmp(name, "lbu") == 0)   return write_mem   (0x24, output, args, num_args);
    else if (strcmp(name, "lw") == 0)    return write_mem   (0x23, output, args, num_args);
    else if (strcmp(name, "sb") == 0)    return write_mem   (0x28, output, args, num_args);
    else if (strcmp(name, "sw") == 0)    return write_mem   (0x2b, output, args, num_args);
    else if (strcmp(name, "beq") == 0)   return write_branch(0x04, output, args, num_args, addr, symtbl);
    else if (strcmp(name, "bne") == 0)   return write_branch(0x05, output, args, num_args, addr, symtbl);
    else if (strcmp(name, "j") == 0)     return write_jump  (0x02, output, args, num_args, addr, reltbl);
    else if (strcmp(name, "jal") == 0)   return write_jump  (0x03, output, args, num_args, addr, reltbl);
    else                                 return -1;
}




/* A helper function for writing most R-type instructions. You should use
   translate_reg() to parse registers and write_inst_hex() to write to 
   OUTPUT. Both are defined in translate_utils.h.

   This function is INCOMPLETE. Complete the implementation below. You will
   find bitwise operations to be the cleanest way to complete this function.
 */
int write_rtype(uint8_t funct, FILE* output, char** args, size_t num_args) {
    // Perhaps perform some error checking?
    if (num_args != 3) return -1;

    int valid_register = 1;
    int rd = translate_reg(args[0]);
    int rs = translate_reg(args[1]);
    int rt = translate_reg(args[2]);
    int shamt = 0;

    valid_register &= is_valid_register(rd);
    valid_register &= is_valid_register(rs);
    valid_register &= is_valid_register(rt);
    if (!valid_register) return -1;

    // printf("rd = %d, rs  = %d, rt = %d \n", rd,rs, rt);
    int instruction = make_rtype_instruction(funct, rd, rs, rt, shamt);
    write_inst_hex(output, instruction);
    return 0;
}


/* A helper function for writing shift instructions. You should use 
   translate_num() to parse numerical arguments. translate_num() is defined
   in translate_utils.h.

   This function is INCOMPLETE. Complete the implementation below. You will
   find bitwise operations to be the cleanest way to complete this function.
 */
int write_shift(uint8_t funct, FILE* output, char** args, size_t num_args) {
	// Perhaps perform some error checking?
  if (num_args != 3) return -1;
  long int shamt;
  int rs = 0;
  int rd = translate_reg(args[0]);
  int rt = translate_reg(args[1]);

  int valid_register = 1;
  valid_register &= is_valid_register(rt);
  valid_register &= is_valid_register(rd);

  int err = translate_num(&shamt, args[2], 0, 31);
  if (err == -1 || !valid_register) return -1;

  uint32_t instruction = make_rtype_instruction(funct, rd, rs, rt, shamt);
  write_inst_hex(output, instruction);
  return 0;
}


int write_jr(uint8_t funct, FILE* output, char** args, size_t num_args) {
  if (num_args != 1) return -1;
  int shamt = 0;
  int rd = 0;
  int rt = 0;
  int rs = translate_reg(args[0]);
  if (!is_valid_register(rs)) return -1;

  uint32_t instruction  = make_rtype_instruction(funct, rd, rs, rt, shamt);
  write_inst_hex(output, instruction);
  return 0;
}



int write_lui(uint8_t opcode, FILE* output, char** args, size_t num_args) {
  if (num_args != 2) return -1;
  char *temp_args[3] = {args[0], "$0", args[1]};
  num_args = 3;
  return write_unsigned_imm(opcode, output, temp_args, num_args);
}



int write_addiu(uint8_t opcode, FILE* output, char** args, size_t num_args) {
  return write_signed_imm(opcode, output, args, num_args);
}


int write_signed_imm(uint8_t opcode, FILE* output, char** args, size_t num_args) {
  if (num_args != 3) return -1;
  long int imm;
  int16_t min  = INT16_MIN; // min signed 16 bit integer
  int16_t max =  INT16_MAX; // max signed 16 bit integer
  int rt = translate_reg(args[0]);
  int rs = translate_reg(args[1]);

  int valid_register = 1;
  valid_register &= is_valid_register(rt);
  valid_register &= is_valid_register(rs);

  int err = translate_num(&imm, args[2], min, max);
  if (err == -1 || !valid_register) return -1;

  uint32_t instruction = make_itype_instruction(opcode, rs, rt, imm);
  write_inst_hex(output, instruction);
  return 0;
}



int write_ori(uint8_t opcode, FILE* output, char** args, size_t num_args) {
   return write_unsigned_imm(opcode, output, args, num_args);
}


int write_unsigned_imm(uint8_t opcode, FILE* output, char** args, size_t num_args) {
  if (num_args != 3) return -1;
  uint16_t max = UINT16_MAX; 
 
  long int imm;
  int rt = translate_reg(args[0]);
  int rs = translate_reg(args[1]);

  int valid_register = 1;
  valid_register &= is_valid_register(rt);
  valid_register &= is_valid_register(rs);

  int err  = translate_num(&imm, args[2], 0, max);
  if (err == -1 || !valid_register) return -1;

  uint32_t instruction = make_itype_instruction(opcode, rs, rt, imm);
  write_inst_hex(output, instruction);
  return 0;
}



int write_mem(uint8_t opcode, FILE* output, char** args, size_t num_args) {
  char *temp_args[3] = {args[0], args[2], args[1]};
  return write_signed_imm(opcode, output, temp_args, num_args);
}







uint32_t make_itype_instruction(uint8_t opcode, int rs, int rt, uint16_t imm) {
  uint32_t instruction = 0;

  instruction <<= 6;
  instruction |= opcode;

  instruction <<= 5; // set the bits of rs field
  instruction |= rs;

  instruction <<= 5; // set the bits of the rt field
  instruction |= rt;

  instruction <<= 16; // set the bits of 16 bit imm field
  instruction |= imm;

  return instruction;
}



int write_branch(uint8_t opcode, FILE* output, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* symtbl) {
  if (num_args != 3) return -1;
  int rs = translate_reg(args[0]);
  int rt = translate_reg(args[1]);

  int valid_register = 1;
  valid_register &= is_valid_register(rt);
  valid_register &= is_valid_register(rs);

  if(!valid_register) return -1;

  char *label = args[2];
  int64_t label_addr = get_addr_for_symbol(symtbl, label);
  if (label_addr == -1) return -1; 

  int64_t offset = label_addr - (addr + 4);
  offset >>= 2;  // calculates the immediate value that will be offset field for branch instruction
  if (!is_valid_signed_16_bit(offset)) return -1;

  uint32_t instruction = make_itype_instruction(opcode, rs, rt, offset);
  write_inst_hex(output, instruction);
  return 0;
}


int write_jump(uint8_t opcode, FILE* output, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* reltbl) {
  if (num_args != 1) return -1;
  
  int err = add_to_table(reltbl, args[0], addr);
  if (err == -1) return -1;
  uint32_t instruction = make_itype_instruction(opcode, 0, 0 , 0);
  write_inst_hex(output, instruction);
  return 0;
}





// int write_itype(uint8_t opcode, FILE* output, char** args, size_t num_args) {
//   if (num_args != 3) return -1;
//   long imm;
//   int16_t min  = INT16_MIN; // min signed 16 bit integer
//   int16_t max =  INT16_MAX; // max signed 16 bit integer
//   int rt = translate_reg(args[0]);
//   int rs = translate_reg(args[1]);
//   int err = translate_num(&imm, args[2], min, max)

// }

int is_valid_register(int val) {
  if (val < 0 || val > 31)
    return 0;
  return 1;
} 


int is_valid_signed_16_bit(long int imm) {
  int16_t min  = INT16_MIN;
  int16_t max =  INT16_MAX;
  if (imm < min || imm > max) return 0;
  return 1;
}




uint32_t make_rtype_instruction(uint8_t funct, int rd, int rs, int rt, int shamt) {
   // printf("rd = %d, rs  = %d, rt = %d \n", rd,rs, rt);

  uint32_t instruction = 0;
  instruction <<= 6; // opcode field is 0

  
  instruction <<= 5; // set the bits of rs field 
  instruction |= rs;
 
  instruction <<= 5; // set the bits of rt field
  instruction |= rt;

  instruction <<= 5; // set the buts of rd field
  instruction |= rd;
  
 
  instruction <<=5; // set shamt bits 
  instruction |= shamt;

  instruction <<=6;
  instruction |= funct; // set the func field bits

  return instruction;
}

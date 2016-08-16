#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include "src/utils.h"
#include "src/tables.h"
#include "src/translate_utils.h"
#include "src/translate.h"

const char* TMP_FILE = "test_output.txt";
const int BUF_SIZE = 1024;

/****************************************
 *  Helper functions 
 ****************************************/

int do_nothing() {
    return 0;
}

int init_log_file() {
    set_log_file(TMP_FILE);
    return 0;
}

int check_lines_equal(char **arr, int num) {
    char buf[BUF_SIZE];

    FILE *f = fopen(TMP_FILE, "r");
    if (!f) {
        CU_FAIL("Could not open temporary file");
        return 0;
    }
    for (int i = 0; i < num; i++) {
        if (!fgets(buf, BUF_SIZE, f)) {
            CU_FAIL("Reached end of file");
            return 0;
        }
        CU_ASSERT(!strncmp(buf, arr[i], strlen(arr[i])));
    }
    fclose(f);
    return 0;
}

/****************************************
 *  Test cases for translate_utils.c 
 ****************************************/

void test_translate_reg() {
    CU_ASSERT_EQUAL(translate_reg("$0"), 0);
    CU_ASSERT_EQUAL(translate_reg("$at"), 1);
    CU_ASSERT_EQUAL(translate_reg("$v0"), 2);
    CU_ASSERT_EQUAL(translate_reg("$a0"), 4);
    CU_ASSERT_EQUAL(translate_reg("$a1"), 5);
    CU_ASSERT_EQUAL(translate_reg("$a2"), 6);
    CU_ASSERT_EQUAL(translate_reg("$a3"), 7);
    CU_ASSERT_EQUAL(translate_reg("$t0"), 8);
    CU_ASSERT_EQUAL(translate_reg("$t1"), 9);
    CU_ASSERT_EQUAL(translate_reg("$t2"), 10);
    CU_ASSERT_EQUAL(translate_reg("$t3"), 11);
    CU_ASSERT_EQUAL(translate_reg("$s0"), 16);
    CU_ASSERT_EQUAL(translate_reg("$s1"), 17);
    CU_ASSERT_EQUAL(translate_reg("$3"), -1);
    CU_ASSERT_EQUAL(translate_reg("asdf"), -1);
    CU_ASSERT_EQUAL(translate_reg("hey there"), -1);
}

void test_translate_num() {
    long int output;

    CU_ASSERT_EQUAL(translate_num(&output, "35", -1000, 1000), 0);
    CU_ASSERT_EQUAL(output, 35);
    CU_ASSERT_EQUAL(translate_num(&output, "145634236", 0, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 145634236);
    CU_ASSERT_EQUAL(translate_num(&output, "0xC0FFEE", -9000000000, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 12648430);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 72), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 71), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 72, 150), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 73, 150), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "35x", -100, 100), -1);
}

/****************************************
 *  Test cases for tables.c 
 ****************************************/

void test_table_1() {
    int retval;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    retval = add_to_table(tbl, "abc", 8);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "efg", 12);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 24); 
    CU_ASSERT_EQUAL(retval, -1); 
    retval = add_to_table(tbl, "bob", 14); 
    CU_ASSERT_EQUAL(retval, -1); 

    retval = get_addr_for_symbol(tbl, "abc");
    CU_ASSERT_EQUAL(retval, 8); 
    retval = get_addr_for_symbol(tbl, "q45");
    CU_ASSERT_EQUAL(retval, 16); 
    retval = get_addr_for_symbol(tbl, "ef");
    CU_ASSERT_EQUAL(retval, -1);
    
    free_table(tbl);

    char* arr[] = { "Error: name 'q45' already exists in table.",
                    "Error: address is not a multiple of 4." };
    check_lines_equal(arr, 2);

    SymbolTable* tbl2 = create_table(SYMTBL_NON_UNIQUE);
    CU_ASSERT_PTR_NOT_NULL(tbl2);

    retval = add_to_table(tbl2, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl2, "q45", 24); 
    CU_ASSERT_EQUAL(retval, 0);

    free_table(tbl2);
}

void test_table_2() {
    int retval, max = 100;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    char buf[10];
    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = add_to_table(tbl, buf, 4 * i);
        CU_ASSERT_EQUAL(retval, 0);
    }

    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = get_addr_for_symbol(tbl, buf);
        CU_ASSERT_EQUAL(retval, 4 * i);
    }

    free_table(tbl);
}
/****************************************
 *  Test cases for translate.c
 ****************************************/

void compare_written_instruction_to(char* str) {
    char line[32];
    FILE* test = fopen("test", "r");
    fgets(line, sizeof(line), test);
    CU_ASSERT(strncmp(line, str, 8) == 0);
    fclose(test);
}

void test_translate_1() {
    // test rtype  and sll instruction translations
    
    char *name;
    int res;
    FILE *test;
    char* args[3] = {"$s0", "$a0", "$t0"};

    
    name = "addu";
    size_t num_args = 3;
    uint32_t addr = 0;
    SymbolTable *symtbl = NULL;
    SymbolTable *reltbl = NULL;

    test = fopen("test", "w");
    res = translate_inst(test, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);   
    compare_written_instruction_to("00888021");


    /***********************/
    name = "sll";
    test = fopen("test", "w");
    res  = translate_inst(test, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);
    args[2] = "16";
    res  = translate_inst(test, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("00048400");


    /***********************/
    name = "jr";
    test = fopen("test", "w");
    res  = translate_inst(test, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    num_args = 1;
    res  = translate_inst(test, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("02000008");

}


void test_translate_2() {
    // test itype and offset based instruction translations
    
    char *name;
    int res;
    FILE *test;

    size_t num_args = 0;
    uint32_t addr = 0;
    SymbolTable *symtbl = NULL;
    SymbolTable *reltbl = NULL;


    /***********************/
    name = "addiu";
    num_args = 1;
    char *args_addiu[3] = {"$s0", "$a0", "$t0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_addiu, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    num_args = 3;
    res  = translate_inst(test, name, args_addiu, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_addiu[2] = "-100";
    num_args = 3;
    res  = translate_inst(test, name, args_addiu, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("2490ff9c");



    /***********************/
    name = "ori";
    num_args = 1;
    char *args_ori[3] = {"$s0", "$a0", "$t0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_ori, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    num_args = 3;
    args_ori[2] = "-100";
    res  = translate_inst(test, name, args_ori, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_ori[2] = "100";
    num_args = 3;
    res  = translate_inst(test, name, args_ori, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("34900064");


    /***********************/
    name = "lui";
    num_args = 2;
    char *args_lui[3] = {"$s0", "$a0", "$t0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_lui, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_lui[1] = "-100";
    res  = translate_inst(test, name, args_lui, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_lui[1] = "100";
    res  = translate_inst(test, name, args_lui, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("3c100064");


    /***********************/
    name = "lb";
    num_args = 3;
    char *args_lb[3] = {"$s0", "$t0", "$a0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_lb, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_lb[1] = "65536";
    res  = translate_inst(test, name, args_lb, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_lb[1] = "-100";
    res  = translate_inst(test, name, args_lb, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("8090ff9c");




    /***********************/
    name = "lbu";
    num_args = 3;
    char *args_lbu[3] = {"$s0", "$t0", "$a0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_lbu, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_lbu[1] = "65536";
    res  = translate_inst(test, name, args_lbu, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_lbu[1] = "-100";
    res  = translate_inst(test, name, args_lbu, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("9090ff9c");


    /***********************/
    name = "lw";
    num_args = 3;
    char *args_lw[3] = {"$s0", "$t0", "$a0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_lw, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_lw[1] = "65536";
    res  = translate_inst(test, name, args_lw, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_lw[1] = "-100";
    res  = translate_inst(test, name, args_lw, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("8c90ff9c");


    /***********************/
    name = "sb";
    num_args = 3;
    char *args_sb[3] = {"$s0", "$t0", "$a0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_sb, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_sb[1] = "-65536";
    res  = translate_inst(test, name, args_sb, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_sb[1] = "100";
    res  = translate_inst(test, name, args_sb, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("a0900064");



    /***********************/
    name = "sw";
    num_args = 3;
    char *args_sw[3] = {"$s0", "$t0", "$a0"};
    test = fopen("test", "w");
    res  = translate_inst(test, name, args_sw, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_sw[1] = "-65536";
    res  = translate_inst(test, name, args_sw, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    args_sw[1] = "-100";
    res  = translate_inst(test, name, args_sw, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    compare_written_instruction_to("ac90ff9c");

}


void test_translate_3() {
    // test branch and jump instruction translations
    
    char *name;
    int res;
    FILE *test;

    size_t num_args = 0;
    uint32_t addr = 0;
    SymbolTable *symtbl = NULL;
    SymbolTable *reltbl = NULL;


    /***********************/
    name = "beq";
    addr = 0;
    symtbl = create_table(SYMTBL_UNIQUE_NAME);
    add_to_table(symtbl, "fib", 0x00400024);
    num_args = 3;
    char *args_beq[3] = {"$s0", "$a0", "$a0"};

    test = fopen("test", "w");
    res  = translate_inst(test, name, args_beq, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_beq[2] = "fib";
    res  = translate_inst(test, name, args_beq, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    addr = 0x00400004;
    res  = translate_inst(test, name, args_beq, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    free_table(symtbl);
    compare_written_instruction_to("12040007");




    /***********************/
    name = "bne";
    addr = 0;
    symtbl = create_table(SYMTBL_UNIQUE_NAME);
    add_to_table(symtbl, "fib", 0x00400024);
    num_args = 3;
    char *args_bne[3] = {"$s0", "$a0", "$a0"};

    test = fopen("test", "w");
    res  = translate_inst(test, name, args_bne, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);

    
    args_bne[2] = "fib";
    res  = translate_inst(test, name, args_bne, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    addr = 0x00400004;
    res  = translate_inst(test, name, args_bne, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    fclose(test);
    free_table(symtbl);
    compare_written_instruction_to("16040007");

    
    /***********************/
    name = "jal";
    addr = 0;
    reltbl = create_table(SYMTBL_UNIQUE_NAME);
    num_args = 3;
    char *args_jal[3] = {"fib", "$a0", "$a0"};

    test = fopen("test", "w");
    res  = translate_inst(test, name, args_jal, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(res, -1);



    num_args = 1;
    addr = 0x00400004;
    res  = translate_inst(test, name, args_jal, num_args, addr, symtbl, reltbl);
    CU_ASSERT_NOT_EQUAL(res, -1);
    CU_ASSERT_EQUAL(get_addr_for_symbol(reltbl, args_jal[0]), addr);
    fclose(test);
    free_table(reltbl);
    compare_written_instruction_to("0c000000");

}





void test_translate_4() {
    // test branch and jump instruction translations
    
    char *name;
    int res;
    FILE *test;
    size_t num_args = 0;

    /***********************/
    name = "li";
    num_args = 2;
    char *args_li[3] = {"$s0", "4294967296"};

    test = fopen("test", "w");
    res  = write_pass_one(test, name, args_li, num_args);
    CU_ASSERT_EQUAL(res, 0);



    args_li[1] = "2147483647";
    res  = write_pass_one(test, name, args_li, num_args);
    CU_ASSERT_EQUAL(res, 2);
    fclose(test);
    compare_written_instruction_to("lui $at 00007fff");

    test = fopen("test", "w");
    args_li[1] = "-100";
    res  = write_pass_one(test, name, args_li, num_args);
    CU_ASSERT_EQUAL(res, 1);
    fclose(test);
    compare_written_instruction_to("addiu $s0 $0 -100");




    /***********************/
    name = "blt";
    num_args = 2;
    char *args_blt[3] = {"$s0", "$a0", "fib"};

    test = fopen("test", "w");
    res  = write_pass_one(test, name, args_blt, num_args);
    CU_ASSERT_EQUAL(res, 0);



    num_args = 3;
    res  = write_pass_one(test, name, args_blt, num_args);
    CU_ASSERT_EQUAL(res, 2);
    fclose(test);
    compare_written_instruction_to("slt $t0 $s0 $a0");


}




/****************************************
 *  Add your test cases here
 ****************************************/

int main(int argc, char** argv) {
    CU_pSuite pSuite1 = NULL, pSuite2 = NULL, pSuite3 = NULL, pSuite4 = NULL ;

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* Suite 1 */
    pSuite1 = CU_add_suite("Testing translate_utils.c", NULL, NULL);
    if (!pSuite1) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_reg", test_translate_reg)) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_num", test_translate_num)) {
        goto exit;
    }

    /* Suite 2 */
    pSuite2 = CU_add_suite("Testing tables.c", init_log_file, NULL);
    if (!pSuite2) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_1", test_table_1)) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_2", test_table_2)) {
        goto exit;
    }

    pSuite3  =  CU_add_suite("Testing translate.c", NULL, NULL);
    if (!pSuite3) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_translate_1", test_translate_1)) {
        goto exit;
    }

    if (!CU_add_test(pSuite3, "test_translate_2", test_translate_2)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_translate_3", test_translate_3)) {
        goto exit;
    }


    pSuite4  =  CU_add_suite("Testing psedoinstructions in translate.c", NULL, NULL);
    if (!pSuite3) {
        goto exit;
    }
    if (!CU_add_test(pSuite4, "test_translate_4", test_translate_4)) {
        goto exit;
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

exit:
    CU_cleanup_registry();
    return CU_get_error();;
}

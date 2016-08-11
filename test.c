#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "src/tables.h"



int main() {
	// long val;
	// errno = 0;    /* To distinguish success/failure after call */
	// char *endptr, *str;
	// int base = 0;

	// str = "35x";
	// val = strtol(str, &endptr, base);

 //   /* Check for various possible errors */

 //   if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
 //            || (errno != 0 && val == 0)) {
 //        perror("strtol");
 //        exit(EXIT_FAILURE);
 //    }

 //    if (endptr == str || *endptr) {
 //        fprintf(stderr, "No digits were found\n");
 //        exit(EXIT_FAILURE);
 //    }

 //    printf("%ld\n",val );

	int retval, max = 100;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
	char buf[10];
    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = add_to_table(tbl, buf, 4 * i);
        assert(retval == 0);
    }

    free_table(tbl);

}
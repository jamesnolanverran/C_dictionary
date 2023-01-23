#include <stdint.h> 
#include <assert.h>
#include <stdarg.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h> 

#include "includes.h"

i32 main(i32 argc, char **argv) {
    // test_dict();

    int *new_dictionary = NULL;
    
    // use the dictionary, keys are char*
    dict_insert(new_dictionary, "new_entry", 33);

    int *result = dict_get(new_dictionary, "new_entry"); 
    printf("result: %d\n", *result);

    *result +=1; // update the dict 

    result = dict_get(new_dictionary, "new_entry"); // to show that it was updated
    printf("result: %d\n", *result);

    dict_free(new_dictionary);

    return 0;
}
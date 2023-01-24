# Dynamic HashMap / Dictionary in C

**This is a work in progress**

- Goal: build a simple python-like dictionary / hash table in C
- This is a personal learning project. I'm learning C and trying out different ideas and idioms I've learned from other programmers. Feedback is welcome. 
- Inspired by Per Vognsen's stretchy buffer implementation in his [Bitwise][playlist] series, which is included here (arr).
- Currently uses MurmerHash2 by Austin Appleby 

[playlist]: https://www.youtube.com/playlist?list=PLU94OURih-CiP4WxKSMt3UcwMSDM3aTtX

## Features

- Dynamic typying
- Dynamic memory allocation
- No boilerplate

### Example: 
```c
    #define DICT_H_IMPLEMENTATION
    include "dict.h"

    // initialize a var with type
    int *new_dictionary = NULL;
    
    // use the dictionary, keys are char*
    dict_insert(new_dictionary, "new_entry", 33);

    int *result = dict_get(new_dictionary, "new_entry"); 
    printf("result: %d\n", *result);

    *result +=1; // update the dict 

    result = dict_get(new_dictionary, "new_entry"); // (to show that it was updated)
    printf("result: %d\n", *result);

    dict_free(new_dictionary);

```
#### TODO: 
- clean up & improve tests
- rethink dependencies
- error handling
- string support for dict
- more docs
- compatibility
- etc

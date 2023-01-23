# C_dictionary
A simple dynamically typed and sized hashmap in C

- Q: Can I build a simple python-like dictionary / hash table in C? 
- This is a personal learning project. I'm learning C and trying out different ideas and idioms I've learned from other programmers. Feedback is welcome. 
- Inspired by Per Vognsen's stretchy buffer implementation in his [Bitwise][playlist] series
- Is it useful? I don't know. I am still new to C. It's useful as a learning project and I can imagine it could be useful to quickly get going on a project with having to worry about boilerplate. 

[playlist]: https://www.youtube.com/playlist?list=PLU94OURih-CiP4WxKSMt3UcwMSDM3aTtX

## Features

- Dynamic typying
- Dynamic memory - reallocates as necessary
- no boilerplate

### Example: 
```c
    include "includes.h"

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
- work in progress, as mentioned above I'm  experimenting with a few ideas
- tyring out single-header file libraries idea in C, see includes.h 
- TODO: clean up & improve tests, rethink type aliasing, error handling, string support, more docs, etc.
- hash function is MurmurHash2, by Austin Appleby

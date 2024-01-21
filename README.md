# C_Dictionary
- C_Dictionary is a hashmap library, designed for fast key-value storage and retrieval. It allows for efficient data lookup and manipulation, making it a versatile tool for a wide range of applications.

> :warning: **Disclaimer: Work in Progress**
- C_Dictionary is currently in a developmental stage. Use at your own risk.
### Developer Friendly Design
- C_Dictionary emphasizes developer velocity, quick iterations, and effortless prototyping.
### Dynamic Types and Memory
- Dynamic typing and dynamic memory offer the ease and flexibility of higher-level languages, while minimizing setup time and eliminating boilerplate.

### Key Flexibility
- C_Dictionary supports keys of any data type without requiring additional boilerplate.

### Contiguous Array
- Data is stored in a contiguous array, useful for bulk processing. This layout ensures fewer cache misses, crucial for high-speed data access and processing.

### Index Stability
- Stable indexing ensures that the indices of elements remain constant after deletions or reallocations which can be important for applications that require persistent references to data elements.

```c
#include "dict.h"  // Include the C_Dictionary header

typedef struct Vector {
    int x;
    int y;
} Vector;

int main() {
    // Initialize two vectors
    Vector vec1 = (Vector){3, 9};
    Vector vec2 = (Vector){5, 25};

    // Create a new dictionary
    Vector *vec_dict = NULL;

    // Insert vectors into the dictionary with keys
    int some_key = 10;
    int another_key = 11;
    dict_insert(vec_dict, some_key, vec1);
    dict_insert(vec_dict, another_key, vec2);

    // Retrieve a vector from the dictionary
    Vector *v = dict_get_ptr(vec_dict, another_key);  
    printf("Vector with key %d: (%d, %d)\n", another_key, v->x, v->y);

    // pointers can become invalid if the hashmap is reallocated. 
    // We use indicies instead
    int index = dict_get(vec_dict, some_key);
    printf("Vector indexed: %d\n", vec_dict[index].x);
    
    // Iterate over all vectors in the dictionary
    printf("All vectors in the dictionary:\n");
    for (int i = 0; i < dict_len(vec_dict); i++) {
        printf("(%d, %d)\n", vec_dict[i].x, vec_dict[i].y);
    }

    // Delete a vector from the dictionary
    int deleted_idx = dict_delete(vec_dict, some_key);
    assert(dict_get(vect_dict, some_key) == -1); // returns -1 if there is no entry found

    // if we intend to iterate over the data we must mark deleted data as invalid
    vec_dict[deleted_idx].x = -1; // mark the Vector invalid
    // Iterate over the dictionary, skipping invalid vectors
    printf("Vectors after deletion:\n");
    for (int i = 0; i < dict_len(vec_dict); i++) {
        if (vec_dict[i].x == -1) continue; // Skip if marked invalid
        printf("(%d, %d)\n", vec_dict[i].x, vec_dict[i].y); 
    }

    // Keys are not stored in the hashmap by default. To store keys, define a struct 
    // that includes both the key and the value, like KeyAndVector below.

    // Example struct to store key and value together:
    // typedef struct KeyAndVector {
    //     int key;
    //     Vector val;
    // } KeyAndVector;

    // Initialize a hashmap to store KeyAndVector items.
    KeyAndVector *key_val_dict = NULL;

    // Insert a key-value pair into the hashmap.
    dict_insert(key_val_dict, some_key, (KeyAndVector){some_key, vec1});
    
    //////////////////
    //////////////////

    dict_clear(vec_dict); // clear/reset the dict without freeing memory

    // using strings for keys
    char kstr1[] = "firstkey";
    char kstr2[] = "the second key";
    int str_idx1 = dict_keystr_insert(vec_dict, kstr1, vec1, strlen(kstr1));
    dict_keystr_insert(vec_dict, kstr2, vec2, strlen(kstr2));
    assert(vec_dict[str_idx1].x == vec1.x);

    int str_get_result = dict_keystr_get(vec_dict, kstr1, strlen(kstr1)); // get data/val index
    assert(vec_dict[str_get_result].x == vec1.x);
    Vector *v2 = dict_keystr_get_ptr(vec_dict, kstr1, strlen(kstr1)); // get ptr to data/val
    assert(v2->x == vec1.x);

    printf("All vectors in the keystr dictionary:\n");
    for (int i = 0; i < dict_range(vec_dict); i++) {
        if (vec_dict[i].x == -1) continue; // Skip if marked invalid
        printf("(%d, %d)\n", vec_dict[i].x, vec_dict[i].y);
    }
    str_idx1 = dict_keystr_delete(vec_dict, kstr2, strlen(kstr2));
    vec_dict[str_idx1].x = -1;

    printf("All vectors in the keystr dictionary after deletion:\n");
    for (int i = 0; i < dict_range(vec_dict); i++) {
        if (vec_dict[i].x == -1) continue; // Skip if marked invalid
        printf("(%d, %d)\n", vec_dict[i].x, vec_dict[i].y);
    }

    // Free the dictionary
    dict_free(vec_dict);

    return 0;
}
```
## How It Works: C_Dictionary

### Stretchy Buffer Technique
C_Dictionary uses the stretchy buffer technique, incorporating struct headers, macros, and Flexible Array Members. This enables dynamic typing and memory management with minimal boilerplate. The hashmap buffer stores hashes and indices, where the indices point to data within a separate flexible array member. 

### Macro Implementation
- The library's macros are structured to output expressions, utilizing the comma operator and ternary expressions. This design enables them to effectively return values. The temp_idx field in the Dictionary header facilitates indirect index communication between functions within a macro. This avoids the need for direct variable passing, ensuring that the macros function as expressions rather than statements.

### Dictionary Declaration and Initialization
- Declare a dictionary with just a type, like int *my_int_dict = NULL. Initial allocation and setup are automatically performed on the first insert, returning the flexible array member (e.g., int* in this case). This design simplifies usage and manages memory efficiently.

### Data Retrieval and Reallocation
- Use dict_get to obtain an index, ideal for scenarios where dynamic reallocation could invalidate pointers. For immediate data access, dict_get_ptr() returns a direct pointer. When handling indices, access data by treating the dictionary handle as an array.

### Hashing and Key Typing
- Keys can be of any type, interpreted as raw bytes from void pointers. This approach trades some type safety, a less critical aspect for keys, for simplicity and versatility in use. C_Dictionary uses MurmerHash2.

### String Key Management
String keys are supported with specialized functions: dict_keystr_insert, dict_keystr_get, dict_keystr_get_ptr, dict_keystr_delete. This approach accommodates string's variable lengths by allowing users to specify the length.

### Deletion and Memory Management
- Deletion flags an entry as deleted without erasing the data, reallocating its slot to a free list. Subsequent inserts use these free slots before expanding the data array. Manual data invalidation is required for direct data array iteration. This method preserves index stability.

### Drawbacks
- Risk of pointer invalidation from data reallocation; using indices is safer.
- Direct data iteration post-deletion needs manual data invalidation; consider a helper function for automation.
- Indirection due to separate hash and data arrays.

## Todos
- 'index unstable' deletion option for data array with no invalid slots.
- dict_init() - optional function to allow for user specified initial allocation, load factor, and other settings.
- option to allow users to use a hashing function of their choice.

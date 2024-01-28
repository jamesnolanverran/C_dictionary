#ifndef DICT_H
#define DICT_H
#include <stdbool.h> 

typedef struct DictEntry { 
    int data_index;
    int hash; 
} DictEntry;

typedef struct DictHdr {
    int len; 
    int cap;
    int temp_idx; // stores an index used by macros
    int *free_list; // arr of indices to *entries that have been deleted. 
    DictEntry *entries; // the actual hashtable - contains an index to data[] where the values are stored and a hash
    char padding; // some padding for alignment
    char data[];  // aligned data array - where the raw data is stored
} DictHdr;

///////////////////////
// These functions are internal but are utilized by macros so need to be declared here.
///////////////////////
DictHdr *dict__hdr(void *d);
int dict__get(void *dict, void *key, size_t key_size);
int dict__delete(void *dict, void *key, size_t key_size);
bool dict__insert_entry(void *dict, void *key, size_t key_size);
bool dict__find_entry(void *dict, void *key, size_t key_size);
void *dict__grow(void *dict, int new_cap, size_t elem_size) ;
void dict__free(void *dict);
///////////////////////
///////////////////////
// Declarations:
static inline int dict_count(void *d);
static inline int dict_cap(void *d);

// Helper Macros - Utilized by other macros.
// =========================================
#define dict__temp_idx(d) (dict__hdr(d)->temp_idx) // 0xffffffffui32 (EMPTY) 
// dict__entries: Retrieves the entries of the dictionary 'd'. 
// Note: caller should ensure 'd' is valid
#define dict__entries(d) (dict__hdr(d)->entries)
// dict__idx_to_val: Retrieves the value corresponding to the index 'idx' in dictionary 'd'. 
// Note: caller should ensure 'd' is valid
#define dict__idx_to_val(d,idx) ( (d)[ dict__entries(d)[(idx)].data_index ] )  
// if the number of entries is more than half the size of the capacity, double the capacity.
#define INITIAL_CAPACITY 64
#define dict__fit(d, n) ((n) * 2 < dict_cap(d) ? 0 : ((d) = dict__grow((d), ((d) ? 2 * dict_cap(d) : INITIAL_CAPACITY), sizeof(*(d)))))
////////////////////////////////////////////
////////////////////////////////////////////

// returns the index in the data array where the value is stored. If key exists returns -1. 
// param - d: pointer to array of v's
//         k: ptr to key of any type
//         v: value of any chosen type
#define dict_insert(d, k, v) (dict__fit((d), dict_count(d)), (dict__insert_entry((d), (k), sizeof(*(k))) ? ((d)[dict__hdr(d)->temp_idx] = (v)), dict__hdr(d)->temp_idx : -1)) 
// same as above but uses a string as key value
// todo: test
#define dict_keystr_insert(d, k, v, key_size) (dict__fit((d), dict_count(d)), (dict__insert_entry((d), (k), (key_size)) ? ((d)[dict__hdr(d)->temp_idx] = (v)), dict__hdr(d)->temp_idx : -1)) 

// dict_get_ptr: Retrieves a pointer to the value associated with the key 'k' in dictionary 'd'.
// Parameters:
// - 'd' is the dictionary from which to retrieve the value.
// - 'k' is the key for the value. The address of 'k' and its size (using sizeof) are used in the lookup.
// Returns: A pointer to the value corresponding to 'k' in 'd', or NULL if the key is not found.
// Note: This macro leverages dict__find_entry for key lookup. If the key is found, it returns the address 
// of the value using dict__idx_to_val; otherwise, it returns NULL.
#define dict_get_ptr(d,k) (dict__find_entry((d), (k), sizeof(*(k))) ? &dict__idx_to_val((d), dict__temp_idx(d)) : NULL)  
// dict_keystr_get: Retrieves a pointer to the value associated with a C-string key 'k' in dictionary 'd'.
// Parameters:
// - 'd' is the dictionary from which to retrieve the value.
// - 'k' is the C-string key for the value.
// - 'key_size' is the size of the key 'k'.
// Returns: A pointer to the value corresponding to 'k' in 'd', or NULL if the key is not found.
// Note: This macro uses dict__find_entry to search for the key. If the key is found, it returns 
// the address of the value using dict__idx_to_val; otherwise, it returns NULL.
#define dict_keystr_get_ptr(d,k, key_size) (dict__find_entry((d), (k), (key_size)) ? &dict__idx_to_val((d), dict__temp_idx(d)) : NULL)  

// Useful? untested
#define dict_get_or_set_default(d,k,v) ( dict__find_entry((d), (k), sizeof(*(k))) ?               \
                               ( &dict__idx_to_val((d), dict__temp_idx(d))) :                     \
                               ( (dict_insert((d), (k), (v))), dict_get_ptr((d), (k))) )

// returns the data index of deleted item or EMPTY (-1 for ints). The user should mark this 
// data as invalid in some way if the user intends to iterate over the data array.

#define dict_delete(d,k) (dict__delete(d, k, sizeof(*(k)))) // returns index to deleted data

#define dict_get(d,k) (dict__get(d, k, sizeof(*(k)))) // returns index to data

#define dict_free(d) ((d) ? (dict__free(d), (d) = NULL, 1) : 0)

static inline int dict_count(void *d){ return d ? dict__hdr(d)->len : 0; } // how many valid entries in the dicctionary; not for iterating directly over the data 
static inline int dict_cap(void *d){ return d ? dict__hdr(d)->cap : 0; }

int dict_keystr_get(void *dict, void *key, size_t key_size); // same as dict_get but for keys that are strings
int dict_keystr_delete(void *dict, void *key, size_t key_size); // returns index to deleted data
int dict_range(void *dict); // for iterating over the data array
void dict_clear(void *dict);

#endif /* DICT_H */

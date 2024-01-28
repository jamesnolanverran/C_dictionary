#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h> 
#include "lib/darr.h"
#include "dict.h"

#define stmnt(s) do { s } while (0)
#define assert_break() (*(int*)0 = 0)
#define assert(x) stmnt(if (!(x)) { assert_break(); })

#define EMPTY 0xffffffffui32 //UINT32_MAX maybe use -1 now that I've switched to ints.
#define DELETED (0xffffffffui32 - 1)
#define KEY_ALREADY_EXISTS (0xffffffffui32 - 2)
#define offset_of(type, member) ((size_t) &(((type *)0)->member))

// void *dict_malloc(size_t num_bytes) {
//     void *ptr = (void*)malloc(num_bytes);
//     if (!ptr) {
//         perror("dict_malloc failed");
//         exit(1);
//     }
//     return ptr;
// }

DictHdr *dict__hdr(void *d){
    return (DictHdr *)( (char *)d - offset_of(DictHdr, data) - *((char*)d - 1) ) ;
}

static int dict__murmer_hash_2 ( const void *key, size_t len, int seed ) {
    const int m = 0x5bd1e995;
    const int r = 24;
    int h = seed ^ (int)len;
    const unsigned char *data = (const unsigned char *)key;
    while(len >= 4) {
        int k = *(int*)data;
        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }
    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
        h *= m;
    };
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
} 
static int dict__generate_hash(void *key, size_t key_size) {
    return dict__murmer_hash_2(key, key_size, 5381);
}
static int dict__find_empty_slot(DictEntry *entries, int hash, int capacity){
    int idx = hash % capacity;
    int j = capacity;
    while(true){
        if( j-- == 0) assert(false); // unreachable
        if(entries[idx].data_index == EMPTY || entries[idx].data_index == DELETED){
            return idx;
        }
        if(entries[idx].hash == hash){
            return KEY_ALREADY_EXISTS;
        }
        idx += 1;
        if(idx >= capacity){
            idx = 0;
        }
    }
}
// Grows the entry array of the dictionary to accommodate more elements.
static void dict__grow_entries(void *dict, int new_cap, size_t elem_size) {
    DictHdr *d = dict__hdr(dict); // Retrieve the dictionary header
    int new_size = new_cap * (int)elem_size; // Calculate the new size in bytes for the entries
    DictEntry *new_entries = malloc(new_size); // Allocate new memory for the entries
    if (!new_entries) {
        perror("malloc failed");
        exit(1);
    }
    memset(new_entries, 0xff, new_size); // Initialize all bits to 1 (used for EMPTY marker)
    // If the dictionary has existing entries, rehash them into the new entry array
    if (dict_count(dict)) {
        for (int i = 0; i < d->cap; i++) {
            if(d->entries[i].data_index == EMPTY) continue; // Skip empty entries
            if(d->entries[i].data_index == DELETED) continue; // Skip deleted entries
            // Find a new empty slot for the entry and update its position
            int new_index = dict__find_empty_slot(new_entries, d->entries[i].hash, new_cap);
            new_entries[new_index] = d->entries[i];
        }
    }
    // Replace the old entry array with the new one
    if (d->entries) {
        free(d->entries);
    }
    d->entries = new_entries;
}

// Grows the dictionary to a new capacity.
void *dict__grow(void *dict, int new_cap, size_t elem_size) {
    int data_cap = new_cap; // The new capacity for data array
    // Calculate size needed for the new header, accounting for data and alignment padding
    size_t data_size = offset_of(DictHdr, data) + (data_cap * elem_size) + 16;
    DictHdr *new_hdr;
    // Allocate a new header if the dictionary is empty, or resize the existing one
    if (!dict) {
        new_hdr = malloc(data_size);
        if (!new_hdr) {
            perror("malloc failed");
            exit(1);
        }
        // Initialize the new header
        new_hdr->len = 0;
        new_hdr->temp_idx = EMPTY;
        new_hdr->entries = NULL;
        new_hdr->free_list = NULL;
    } else {
        new_hdr = realloc(dict__hdr(dict), data_size);
        if (!new_hdr) {
            perror("realloc failed");
            exit(1);
        }
    }

    // Calculate and apply alignment padding to the data array
    char alignment_padding = (16 - ((uintptr_t)new_hdr->data & 15)) & 15; // Align data[]
    char *aligned_data = new_hdr->data + alignment_padding;
    *(aligned_data - 1) = alignment_padding; // Store the amount of padding

    // Grow the entries to fit into the newly allocated space
    dict__grow_entries(aligned_data, new_cap, sizeof(DictEntry));
    new_hdr->cap = new_cap;
    return aligned_data; // Return the aligned data pointer
}
void dict__free(void *dict){
    DictHdr *d = dict__hdr(dict);
    if(d){
        if(d->entries) {
            free(d->entries); 
        }
        if(d->free_list) darr_free(d->free_list);
        free(dict__hdr(dict));
    }
}
// clear/reset the dict without freeing memory
void dict_clear(void *dict){
    if(!dict) return;
    DictHdr *d = dict__hdr(dict);
    darr_clear(d->entries);
    darr_clear(d->free_list);
    d->len = 0;
}
bool dict__insert_entry(void *dict, void *key, size_t key_size){ 
    DictHdr *d = dict__hdr(dict);
    int data_index = dict_count(dict);
    int hash = dict__generate_hash(key, key_size);
    int entry_index = dict__find_empty_slot(d->entries, hash, dict_cap(dict));
    if(entry_index == KEY_ALREADY_EXISTS){
        return false;
    }
    d->entries[entry_index] = (DictEntry){data_index, hash};  
    // we use a free list to keep track of empty slots in the data array from deletions. Use those first. 
    d->temp_idx = darr_len(d->free_list) ? darr_pop(d->free_list) : d->len;
    d->len += 1;
    return true;
}
// Function: dict__get_entry_index
// Description: Searches for a key in the dictionary and returns its index if found.
//              This function is used internally for operations like insertions or deletions.
// Parameters:
//   void *dict - Pointer to the dictionary in which to search for the key.
//   void *key - Pointer to the key to be searched.
//   size_t key_size - Size of the key.
// Returns:
//   int - The index of the entry where the key is found, or -1 if the key is not present.
static int dict__get_entry_index(void *dict, void *key, size_t key_size){
    if(dict_cap(dict)==0) return -1; // Check if the dictionary is empty or NULL
    DictHdr *d = dict__hdr(dict); // Retrieve the header of the dictionary for internal structure access.
    int hash = dict__generate_hash(key, key_size); // Generate a hash value for the given key.
    int idx = hash % d->cap; // Calculate the initial index to start the search in the hash table.
    int j = d->cap; // Counter to ensure the loop doesn't iterate more than the capacity of the dictionary.

    while(true) { // Loop to search for the key in the dictionary.
        if(j-- == 0) assert(false); // Fail-safe to avoid infinite loops. Should be unreachable if logic is correct.
        if(d->entries[idx].data_index == EMPTY) {  // If the entry is empty, the key is not in the dictionary.
            return -1;
        }
        if(d->entries[idx].hash == hash) { // If the hash matches, the correct entry has been found.
            return idx;
        }
        idx += 1; // Move to the next index, wrapping around to the start if necessary.
        if(idx >= d->cap) { idx = 0; } 
    }
}
// Function: dict__find_entry
// Description: Searches for an entry in the dictionary corresponding to the given key.
//              If found, stores the index of this entry in a temporary variable within
//              the dictionary header structure.
// Parameters:
//   void *dict - Pointer to the dictionary in which to search for the entry.
//   void *key - Pointer to the key for which the entry is to be found.
//   size_t key_size - Size of the key.
// Returns:
//   bool - True if the entry is found, False otherwise.
bool dict__find_entry(void *dict, void *key, size_t key_size){
    int idx = dict__get_entry_index(dict, key, key_size);
    if(idx == -1) return false; // entry is not found

    DictHdr *d = dict__hdr(dict);
    // Store the found index in the temporary variable within the dictionary header.
    // This is necessary for subsequent macro operations that require this index.
    d->temp_idx = idx;
    return true;
}
// Function: dict_get
// Description: Retrieves the index of the data associated with a given key in a dictionary.
// Parameters:
//   void *dict - Pointer to the dictionary from which the data index is to be retrieved.
//   void *key - Pointer to the key for which the data index is required.
//   size_t key_size - Size of the key.
// Returns:
//   int - The index of the data associated with the key, or -1 if the key is not found.
int dict__get(void *dict, void *key, size_t key_size){
    int idx = dict__get_entry_index(dict, key, key_size);
    if(idx == -1) return -1;
    DictHdr *d = dict__hdr(dict);
    return d->entries[idx].data_index;
}
int dict__delete(void *dict, void *key, size_t key_size){
    int idx = dict__get_entry_index(dict, key, key_size);
    if(idx == -1) return -1;
    DictHdr *d = dict__hdr(dict);
    int data_index = d->entries[idx].data_index;
    d->entries[idx].data_index = DELETED;
    darr_push(d->free_list, data_index);
    d->len -= 1; 
    // return the data index of the deleted entry. Caller may wish to mark data as invalid
    return data_index;
}
int dict_keystr_delete(void *dict, void *key, size_t key_size){
    return dict__delete(dict, key, key_size);
}
int dict_keystr_get(void *dict, void *key, size_t key_size){
    return dict__get(dict, key, key_size);
}
// len of the data array, including invalid entries. For iterating
int dict_range(void *dict){ 
    return dict ? dict__hdr(dict)->len + darr_len(dict__hdr(dict)->free_list) : 0; 
} 

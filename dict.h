#ifndef DICT_H
#define DICT_H

typedef struct DictEntry { 
    u32 data_index;
    u32 keys_index;
    u32 entry_indices_index; // added to speed up deletions / avoid linear search
    u32 hash; 
} DictEntry;

typedef struct DictHdr {
    u32 len; 
    u32 cap;
    u32 data_cap; // not required, used for debugging allocation
    u32 temp_idx; // used to store an index used by macros

    u32 *deleted_entry_indices;  // TODO: not implemented
    u32 *deleted_data_indices;// will re-use slots in the dict after deletions
    u32 *deleted_keys; 

    u32 *entry_indices; // an arr of all dict entry locations by index, used for realloc 
    DictEntry *entries; // the actual hashtable - contains an index to data[]  
    char **keys; // arr of c-strings
    char data[];  // where the raw data is stored
} DictHdr;

u32 murmer_hash_2 ( const void * key, u32 len, uint32_t seed );
u32 generate_hash(char *str);
void dict__delete(void *dict);
u32 dict__find_empty_slot(DictEntry *entries, u32 hash, u32 capacity);
bool dict__insert_entry(void *dict, char *key);
u32 dict__index(u32 capacity, char *key);
bool dict__find_hash(void *dict, char *key);
void dict__grow_entries(void *dict, size_t elem_size);
void *dict__grow(void *dict, u32 new_cap, size_t elem_size) ;
bool dict_key_exists(void *dict, char *key);
char **dict_keys(void *dict);
void test_dict(void);

/////////////////////////////
#ifdef DICT_H_IMPLEMENTATION
/////////////////////////////

#define dict__hdr(d) ((DictHdr *)((char *)(d) - offsetof(DictHdr, data))) 
#define dict_temp_idx(d) ((d) ? dict__hdr(d)->temp_idx : EMPTY) 
#define dict_cap(d) ((d) ? dict__hdr(d)->cap : 0)
#define dict_keys_arr(d) ((d) ? dict__hdr(d)->keys : 0)
#define dict_entries(d) ((d) ? dict__hdr(d)->entries : 0)

#define dict_idx_to_val(d,idx) ( (d)[ dict_entries(d)[idx].data_index ] )  
#define dict_key(d,idx) ( dict_keys_arr(d)[ dict_entries(d)[idx].keys_index ] )  // dict_idx_to_key?  todo: needed?

#define dict_len(d) ((d) ? dict__hdr(d)->len : 0)
#define dict_end(d) ((d) + dict_len(d)) // todo: needed? 
#define dict_indices(d) ((d) ? dict__hdr(d)->entry_indices : 0) 
#define dict_free(d) ((d) ? (dict__free(d), (d) = NULL, 1) : 0)
#define dict_fit(d, n) (((n)*3)+1 < dict_cap(d) ? 0 : ((d) = dict__grow((d), ((d) ? 4*dict_cap(d) : 4), sizeof(*(d)))))

#define dict_insert(d, k, v) ((dict_fit((d), dict_len(d))), (dict__insert_entry((d), (k)) ? ((d)[dict__hdr(d)->len++] = (v)), true : false)) 
    // returns bool - false if key already exists

#define dict_get(d,k) (dict__find_hash((d), (k)) ? &dict_idx_to_val((d), dict_temp_idx(d)) : NULL)  // returns ptr to value or NULL
#define dict_update(d,k,v) ( dict__find_hash((d), (k)) ? *(&dict_idx_to_val((d), dict_temp_idx(d))) = (v), true : false ) // false if key doesn't exist
  // do I need this? I can update the value directly after via dict_get
#define dict_delete(d,k) (dict__find_hash((d), (k)) ? dict__delete((d)), true : false) 

#define dict_to_arr(dest_arr, src_dict) do {                                            \
    DictHdr *dh = dict__hdr(src_dict);                                                   \
    for (u32 *idx = dh->entry_indices; idx !=arr_end(dh->entry_indices); idx++) {         \
        if(*idx != DELETED) {                                                           \
            arr_push((dest_arr), (src_dict)[dh->entries[*idx].data_index]);              \
        }                                                                               \
    }                                                                                   \
} while (0)

#define dict_get_or_set(d,k,v) ( dict__find_hash((d), (k)) ? \
                                 (&dict_idx_to_val((d), dict_temp_idx(d))) : \
                                  ((dict_insert((d), (k), (v))), dict_get((d), (k))) )

u32 murmer_hash_2 ( const void * key, u32 len, uint32_t seed )
{
  const uint32_t m = 0x5bd1e995;
  const int r = 24;
  u32 h = seed ^ len;
  const unsigned char * data = (const unsigned char *)key;
  while(len >= 4)
  {
    uint32_t k = *(uint32_t*)data;
    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }
  switch(len)

  {
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
u32 generate_hash(char *str)
{
    return murmer_hash_2(str, (u32)strlen(str), 5381);
}

void dict__delete(void *dict) {  // return a success fail code?  
    DictHdr *d = dict__hdr(dict);

    DictEntry *entry = &d->entries[d->temp_idx];
    arr_push(d->deleted_data_indices, entry->data_index); // TODO: not implemented: recycling slots, & sync lengths of deleted nodes etc.
    arr_push(d->deleted_entry_indices, entry->entry_indices_index);
    arr_push(d->deleted_keys, entry->keys_index);

    d->keys[entry->keys_index] = NULL;
    d->entry_indices[entry->entry_indices_index] = DELETED;

    entry->data_index = DELETED;
    entry->keys_index = DELETED;
    entry->entry_indices_index = DELETED;
    entry->hash = DELETED;

    // d->len--; //  todo: the len no longer reflects how many items are stored.
    d->temp_idx = EMPTY;
}

u32 dict__find_empty_slot(DictEntry *entries, u32 hash, u32 capacity){
    u32 idx = hash % capacity;
    u32 j = capacity;
    u32 existing_hash; 
    while(true){
        if( j-- == 0) assert(false); // unreachable

        existing_hash = entries[idx].hash;

        if(existing_hash == hash){
            return KEY_ALREADY_EXISTS;
        }
        if(existing_hash == EMPTY || existing_hash == DELETED){
            return idx;
        }
        idx += 1;
        if(idx > capacity){
            printf("(dict_get): wrap idx: %d, cap: %d\n", idx, capacity);
            idx = 0;
        }
    }
    return idx;
}
bool dict__insert_entry(void *dict, char *key){ 
    DictHdr *d = dict__hdr(dict);
    u32 data_index = dict_len(dict);
    u32 key_index = arr_len(d->keys);
    u32 hash = generate_hash(key);
    u32 entry_index = dict__find_empty_slot(d->entries, hash, dict_cap(dict));
    if(entry_index == KEY_ALREADY_EXISTS){
        return false;
    }
    u32 entry_indices_index = arr_len(d->entry_indices);
    arr_push(d->entry_indices, entry_index); // keep a list of dict entries
    char *new_key = NULL;
    arr_printf(new_key, key);
    arr_push(d->keys, new_key); // keep a list of keys
    d->entries[entry_index] = (DictEntry){data_index, key_index, entry_indices_index, hash};  
    return true;
}
u32 dict__index(u32 capacity, char *key){ 
    u32 h;
    h = generate_hash(key);
    h = h % capacity;
    return h; 
}
bool dict__find_hash(void *dict, char *key){
    // puts the result in d->temp_idx and returns true. Otherwise false.
    DictHdr *d = dict__hdr(dict);
    u32 hash = generate_hash(key);
    u32 idx = hash % d->cap;
    u32 j = d->cap;
    u32 candidate_hash;
    while(true) {
        if( j-- == 0) assert(false); // unreachable
        candidate_hash = d->entries[idx].hash;
        if(candidate_hash == EMPTY) {  
            d->temp_idx = EMPTY;
            return false; 
        }
        if(candidate_hash == hash) { 
            d->temp_idx = idx;
            return true;
        }
        idx += 1;
        if(idx > d->cap) { idx = 0; } 
    }
}

void dict__grow_entries(void *dict, size_t elem_size){
    DictHdr *d = dict__hdr(dict);
    u32 new_cap = dict_cap(dict);
    u32 new_size = new_cap * (u32)elem_size;
    assert(new_size % 4 == 0);
    DictEntry *new_entries = (DictEntry*)xmalloc(new_size);
    memset(new_entries, 0xff, new_size); 
    if (dict_len(dict)) { 
        u32 *new_entry_indices = NULL;
        for (u32 *it = d->entry_indices; it !=arr_end(d->entry_indices); it++) {
            u32 new_index = dict__find_empty_slot(new_entries, d->entries[*it].hash, new_cap);
            new_entries[new_index] = d->entries[*it];
            arr_push(new_entry_indices, new_index); 
        }
        d->entry_indices = new_entry_indices;
    }
    d->entries = new_entries;
}
void *dict__grow(void *dict, u32 new_cap, size_t elem_size) {
    assert(dict_cap(dict) <= (SIZE_MAX - 1)/2); 
    assert(new_cap <= (SIZE_MAX - offsetof(DictHdr, data))/elem_size);
    u32 data_cap = new_cap / 3;  
    size_t data_size = offsetof(DictHdr, data) + (data_cap * elem_size);
    assert(data_size % 4 == 0);
    DictHdr *new_hdr;
    if (!dict) { 
        new_hdr = (DictHdr*)xmalloc(data_size);
        new_hdr->len = 0;
        new_hdr->temp_idx = EMPTY;
        new_hdr->deleted_data_indices = NULL;
        new_hdr->deleted_keys = NULL;
        new_hdr->deleted_entry_indices = NULL;
        new_hdr->entry_indices = NULL;
        new_hdr->entries = NULL;
        new_hdr->keys = NULL;
    } else {
        new_hdr = (DictHdr*)xrealloc(dict__hdr(dict), data_size);
    }
    new_hdr->data_cap = data_cap;
    new_hdr->cap = new_cap;
    dict__grow_entries(new_hdr->data, sizeof(DictEntry));
    return new_hdr->data;
}
bool dict_key_exists(void *dict, char *key){
    DictHdr *d = dict__hdr(dict);
    u32 hash = generate_hash(key);
    if(dict__find_empty_slot(d->entries, hash, dict_cap(dict)) == KEY_ALREADY_EXISTS) {
        return true;
    }
    return false;
}
char **dict_keys(void *dict){  
    DictHdr *d = dict__hdr(dict);
    char **result = NULL;
    for(char **k = d->keys; k != arr_end(d->keys); k++){ 
        if(*k) arr_push(result, *k);
    }
    return result;
}
void dict__free(void *dict){
    DictHdr *d = dict__hdr(dict);
    if(d){
        if(d->deleted_entry_indices) arr_free(d->deleted_entry_indices);
        if(d->deleted_data_indices) arr_free(d->deleted_data_indices);
        if(d->deleted_keys) arr_free(d->deleted_keys);
        if(d->entry_indices) arr_free(d->entry_indices);
        if(d->keys) arr_free(d->keys);
        if(d->entries) free(d->entries);
        free(dict__hdr(dict));
    }
}


//////////////////////////////////////////////////////
////////////////////////testing///////////////////////
//////////////////////////////////////////////////////
typedef struct testtype {
    u32 x;
    char w;
    u64 y;
    u32 offset;
    char z[128];
} testtype;

void test_dict(void){ 
    testtype *my_dict = {0}; 
    char test_key[4] = {'a', 'a', 'a', '\0'};
    for(u32 i = 1; i < 9000; i++){ 
        dict_insert(my_dict, test_key, (testtype){i*12});
        assert(dict_get(my_dict, (char*)test_key)->x == i*12);
        if(i%1==100) printf("insert: %d\n", i);

        if(test_key[2] == 'z'){
            if(test_key[1] =='z'){
                test_key[2] = 'a';
                test_key[1] = 'a';
                test_key[0]++;
            } else {
                test_key[2] = 'a';
                test_key[1]++;
            }
        } else {
            test_key[2]++;
        }
        //printf("%s\t", test_key);
    }   

    assert(dict_get(my_dict, "false_key") == NULL); 
   /* ============== test delete ======================*/ 
    assert(dict_get(my_dict, "aaa")->x == 12); 
    dict_get(my_dict, "aaa")->x = DELETED;  // user has to invalidate their own data before calling dict_delete
    // this is only necessary if they want to iterate over the raw data, as deleted data is not removed from the array; 
    // instead the slot is simply made available for future inserts. 
    assert(dict_delete(my_dict, "aaa")==true); 
    //assert(dict_delete(my_dict, "abb")==true); 
    assert(dict_delete(my_dict, "this_donut_exist")==false); 
    assert(dict_get(my_dict, "aaa") == NULL); 
    //assert(dict_get(my_dict, "abb")==NULL); 
    
    /* ============== test if key exists ======================*/ 
    assert(dict_insert(my_dict, "aab", (testtype){1000}) == false); // there was a warning here which I think is because I use the comma operator in the macro
    assert(dict_key_exists(my_dict, "aab") == true);                     // so compiler believes I am returning an int; fixed w/ extra parens
    assert(dict_key_exists(my_dict, "zzzzzzz") == false);                     
    assert(dict_key_exists(my_dict, "aaa") == false);                     
    /* ============== dict_get_or_set ====================== */ 
    assert(dict_get_or_set(my_dict, "getorset", (testtype){133})->x == 133);
    assert(dict_get_or_set(my_dict, "getorset", (testtype){133})->x == 133); 
    assert(dict_get_or_set(my_dict, "getorset", (testtype){153})->x != 153);

    /* ============== dict_update ======================*/ 
    DictHdr *d = dict__hdr(my_dict);
    assert(dict_get_or_set(my_dict, "new_dict_update", (testtype){183})->x == 183);
    assert(dict_get(my_dict, "new_dict_update")->x == 183); 
    assert(dict_update(my_dict, "new_dict_update", (testtype){153})==true);
    assert(dict_get(my_dict, "new_dict_update")->x == 153); 
    assert(dict_update(my_dict, "doesnt_exist", (testtype){153})==false); 

   /* ============== dict_keys ======================*/ 
    char **my_keys = dict_keys(my_dict); // test dict_keys
    for(char **it = my_keys; it != arr_end(my_keys); it++){
        printf("%s\t", *it);
    }
    printf("\n");
   /* ============= dict_key and val ==================*/ 
    // name: dict_idx_to_key()??  idx2key idx2val
   assert(strcmp(dict_key(my_dict, *(dict_indices(my_dict)+1)), "aab") == 0); // first entry was deleted
   assert(dict_idx_to_val(my_dict, *(dict_indices(my_dict)+1)).x == 24);
   /* =============== dict_to_arr =====================*/ 

   testtype *dest_arr = NULL;
   dict_to_arr(dest_arr, my_dict);
   assert(dest_arr[0].x == dict_get(my_dict, "aab")->x);
   for(testtype *it = dest_arr; it != arr_end(dest_arr); it++){
        printf("dict_to_arr: %u\t", it->x);
   }

   /* =================================================*/ 
// arr of data is just treated like a normal array ------- see notes on delete test above
    for(testtype *it = my_dict; it != dict_end(my_dict); it++){ 
        if(it->x != DELETED) { 
            printf("%u\t", it->x);
        }
    }
    test_key[0] = 'a';
    test_key[1] = 'a';
    test_key[2] = 'b';
    test_key[3] = '\0';
    for(u32 i = 2; i < 8999; i++){
       assert(dict_get(my_dict, (char*)test_key)->x == i*12);
       if(i%100==0) printf("get: %d\n", i);
        
        if(test_key[2] == 'z'){
            if(test_key[1] =='z'){
                test_key[2] = 'a';
                test_key[1] = 'a';
                test_key[0]++;
            } else {
                test_key[2] = 'a';
                test_key[1]++;
            }
        } else {
            test_key[2]++;
        }
        //if(i%10==0) printf("get %d\n", i);
    }   //*/
  dict_free(my_dict); 
  d = NULL;
  arr_free(my_keys);
  
}
#endif /* DICT_H_IMPLEMENTATION */

#endif /* DICT_H */
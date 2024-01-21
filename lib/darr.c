#include <stdlib.h>  
#include <stdarg.h> 
#include <stdio.h> 
#include "darr.h"

#define stmnt(s) do { s } while (0)
#define assert_break() (*(int*)0 = 0)
#define assert(x) stmnt(if (!(x)) { assert_break(); })

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

DarrHdr *darr__hdr(void *arr) { return (DarrHdr*)( (char*)(arr) - offsetof(DarrHdr, arr) - *((char*)(arr) - 1)); }
int darr_len(void *a) { return a ? darr__hdr(a)->len : 0; }
int darr_cap(void *a) { return a ? darr__hdr(a)->cap : 0; }
void darr_clear(void *a) { if (a)  darr__hdr(a)->len = 0; }
void darr__free(void *a){ free(darr__hdr(a)); }

void *darr_calloc(size_t num_elems, size_t elem_size) {
    void *ptr = calloc(num_elems, elem_size);
    if (!ptr) {
        perror("darr_calloc failed");
        exit(1);
    }
    return ptr;
}
void *darr_realloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("darr_realloc failed");
        exit(1);
    }
    return ptr;
}
void *darr_malloc(size_t num_bytes) {
    void *ptr = (void*)malloc(num_bytes);
    if (!ptr) {
        perror("darr_malloc failed");
        exit(1);
    }
    return ptr;
}
void *darr__init(void *arr, unsigned int initial_capacity, float growth_factor, int elem_size){
    if(arr) assert(0 && "unreachable - darr__init - array already exists");
    assert(growth_factor == 0 || growth_factor > 1.0);

    int new_cap = initial_capacity ? initial_capacity : 16;
    int new_size = offsetof(DarrHdr, arr) + new_cap * elem_size + 16; // add 16 bits for alignment padding

    DarrHdr *new_hdr;
    new_hdr = darr_malloc(new_size);
    new_hdr->len = 0;
    new_hdr->cap = new_cap;
    new_hdr->growth_factor = growth_factor ? growth_factor : 2;
    new_hdr->min_size = new_cap;
    // todo - don't use modulo to align data, use bitwise ops
    char alignment_padding = ((unsigned long long)new_hdr->arr % 16) ? (16 - (unsigned long long)new_hdr->arr % 16) : 0; //align data[]
    char *aligned_data = new_hdr->arr + alignment_padding;
    *(aligned_data - 1) = alignment_padding; // store amount of padding at aligned_data - 1
    return aligned_data;

}
void *darr__grow(void *arr, unsigned int new_len, unsigned int elem_size) {
    assert(darr_cap(arr) <= (SIZE_MAX - 1)/2);

    unsigned int new_cap; 
    unsigned int  min_size = 16;
    float growth_factor = 2.0f;
    if(arr){
        // override defaults
        growth_factor = darr__hdr(arr)->growth_factor > 0 ? darr__hdr(arr)->growth_factor : growth_factor;
        min_size = darr__hdr(arr)->min_size > 0 ? darr__hdr(arr)->min_size : min_size;
    }    
    // new_len is set either from darr_printf or darr_fit
    new_cap = MAX(min_size, MAX((unsigned int)(growth_factor * (float)darr_cap(arr)), new_len)); 

    assert(new_len <= new_cap); 
    int new_size = offsetof(DarrHdr, arr) + (new_cap * elem_size) + 16; // add 16 for alignment padding
    DarrHdr *new_hdr;
    if (arr) {
        new_hdr = darr_realloc(darr__hdr(arr), new_size);
    } else {
        new_hdr = darr_malloc(new_size);
        new_hdr->len = 0;
        new_hdr->growth_factor = growth_factor;
        new_hdr->min_size = min_size;
    }   
    new_hdr->cap = new_cap;
    // todo - don't use modulo to align data, use bitwise ops
    char alignment_padding = (16 - ((unsigned long long)new_hdr->arr & 15)) & 15; // Align data[]
    char *aligned_data = new_hdr->arr + alignment_padding;
    *(aligned_data - 1) = alignment_padding; // store amount of padding at aligned_data - 1
    return aligned_data;
}

char *darr__printf(char *arr, const char *fmt, ...) { 
    va_list args;
    va_start(args, fmt);
    int cap = darr_cap(arr) - darr_len(arr);
    int n = 1 + vsnprintf(darr_end(arr), cap, fmt, args); 
    va_end(args);
    if (n > cap) {
        darr_fit(arr, n + darr_len(arr));
        va_start(args, fmt);
        int new_cap = darr_cap(arr) - darr_len(arr);
        n = 1 + vsnprintf(darr_end(arr), new_cap, fmt, args);
        assert(n <= new_cap);
        va_end(args);
    }
    darr__hdr(arr)->len += n - 1;
    return arr;
}

#ifndef DARR_H
#define DARR_H
// dynamic array
// the dynamic array was originally written by Per Vognsen, from his wonderful Bitwise series:
// https://www.youtube.com/playlist?list=PLU94OURih-CiP4WxKSMt3UcwMSDM3aTtX
// based on Sean Barrett's stretchy buffers https://github.com/nothings/stb

typedef struct DarrHdr { 
    int len;
    int cap;
    float growth_factor; 
    unsigned int min_size; 
    char alignment_padding;
    char arr[]; 
} DarrHdr;

DarrHdr *darr__hdr(void *arr);
void darr__free(void *a);

static inline int darr_len(void *a);
static inline int darr_cap(void *a);
static inline void darr_clear(void *a);

void *darr__grow(void *arr, unsigned int new_len, unsigned int elem_size);
void *darr__init(void *arr, unsigned int initial_capacity, float growth_factor, int elem_size);
char *darr__printf(char *arr, const char *fmt, ...);

#define darr_end(a) ((a) + darr_len(a))
#define darr_free(a) ((a) ? (darr__free(a), (a) = ((void *)0), 1) : 0) // NULL = ((void *)0))
#define darr_fit(a, n) ((n) <= darr_cap(a) ? 0 : ((a) = darr__grow((a), (n), sizeof(*(a)))))

#define darr_push(a, ...) (darr_fit((a), 1 + darr_len(a)), (a)[darr__hdr(a)->len] = (__VA_ARGS__), darr__hdr(a)->len++) // returns idx
#define darr_printf(a, ...) ((a) = darr__printf((a), __VA_ARGS__))
// optional init: allow users to define initial capacity and growth factor
// myarr = darr_init(myarr, 255, 0, 0); //  zero's indicates the user will use default values
#define darr_init(a, initial_capacity, growth_factor) ((a) = darr__init((a), initial_capacity, growth_factor, sizeof(*(a)))) 

#define darr_pop(a) ((a)[darr__hdr(a)->len-- - 1]) 
#define darr_peek(a) ((a)[darr__hdr(a)->len - 1] ) // it's up to the user to null check etc.

static inline int darr_len(void *a) { return a ? darr__hdr(a)->len : 0; }
static inline int darr_cap(void *a) { return a ? darr__hdr(a)->cap : 0; }
static inline void darr_clear(void *a) { if (a)  darr__hdr(a)->len = 0; }

#endif /* DARR_H */

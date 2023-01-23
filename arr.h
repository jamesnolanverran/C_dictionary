#ifndef ARR_H
#define ARR_H
// written by Per Vognsen, from his wonderful Bitwise series:
// https://www.youtube.com/playlist?list=PLU94OURih-CiP4WxKSMt3UcwMSDM3aTtX
// Based on Sean Barrett's stretchy buffers https://github.com/nothings/stb
typedef struct ArrHdr {
    u32 len;
    u32 cap;
    char arr[0]; 
    
} ArrHdr;

void *arr__grow(void *arr, u32 new_len, u32 elem_size);
char *arr__printf(char *arr, const char *fmt, ...);
void arr_test(void);

///////////////////////////
#ifdef ARR_H_IMPLEMENTATION
///////////////////////////

#define arr__hdr(a) ((ArrHdr *)((char *)(a) - offsetof(ArrHdr, arr))) 

#define arr_len(a) ((a) ? arr__hdr(a)->len : 0)
#define arr_cap(a) ((a) ? arr__hdr(a)->cap : 0)
#define arr_end(a) ((a) + arr_len(a))
#define arr_sizeof(a) ((a) ? arr_len(a)*sizeof(*a) : 0)

#define arr_free(a) ((a) ? (free(arr__hdr(a)), (a) = NULL) : 0)
#define arr_fit(a, n) ((n) <= arr_cap(a) ? 0 : ((a) = arr__grow((a), (n), sizeof(*(a)))))
#define arr_push(a, ...) (arr_fit((a), 1 + arr_len(a)), (a)[arr__hdr(a)->len++] = (__VA_ARGS__))

#define arr_printf(a, ...) ((a) = arr__printf((a), __VA_ARGS__))
#define arr_clear(a) ((a) ? arr__hdr(a)->len = 0 : 0)
#define arr_pop(a) ((a)[arr__hdr(a)->len-- - 1]) 
#define arr_peek(a) ((a)[arr__hdr(a)->len - 1] )
#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

void *arr__grow(void *arr, u32 new_len, u32 elem_size) {
    assert(arr_cap(arr) <= (SIZE_MAX - 1)/2);
    u32 new_cap = MAX(16, MAX(1 + 2*arr_cap(arr), new_len));
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(ArrHdr, arr))/elem_size);
    u32 new_size = offsetof(ArrHdr, arr) + new_cap*elem_size;
    new_size = new_size + (new_size % 2);
    ArrHdr *new_hdr;
    if (arr) {
        new_hdr = xrealloc(arr__hdr(arr), new_size);
    } else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }   
    new_hdr->cap = new_cap;
    return new_hdr->arr;
}

char *arr__printf(char *arr, const char *fmt, ...) { 
    va_list args;
    va_start(args, fmt);
    u32 cap = arr_cap(arr) - arr_len(arr);
    u32 n = 1 + vsnprintf(arr_end(arr), cap, fmt, args); 
        va_end(args);
    if (n > cap) {
        arr_fit(arr, n + arr_len(arr));
        va_start(args, fmt);
        u32 new_cap = arr_cap(arr) - arr_len(arr);
        n = 1 + vsnprintf(arr_end(arr), new_cap, fmt, args);
        assert(n <= new_cap);
        va_end(args);
    }
    arr__hdr(arr)->len += n - 1;
    return arr;
}
#endif /* ARR_H_IMPLEMENTATION */

#endif /* ARR_H */
#ifndef COMMON_H
#define COMMON_H

void *xcalloc(size_t num_elems, size_t elem_size);
void *xrealloc(void *ptr, size_t num_bytes);
void *xmalloc(size_t num_bytes);

/////////////////////////////
#ifdef COMMON_H_IMPLEMENTATION
/////////////////////////////

#define EMPTY U32_MAX
#define DELETED (U32_MAX -1)
#define ENTRY_NOT_FOUND (U32_MAX -2)
#define KEY_ALREADY_EXISTS (U32_MAX -3)

#define kiloBytes(value) ((value)*1024)
#define megaBytes(value) (kiloBytes(value)*1024)
#define gigaBytes(value) (megaBytes((i32) value)*1024)

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)
#define IS_POW2(x) (((x) != 0) && ((x) & ((x)-1)) == 0)
#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

void *xcalloc(size_t num_elems, size_t elem_size) {
    void *ptr = calloc(num_elems, elem_size);
    if (!ptr) {
        perror("xcalloc failed");
        exit(1);
    }
    return ptr;
}
void *xrealloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}
void *xmalloc(size_t num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

#endif /* COMMON_H_IMPLEMENTATION */

#endif /* COMMON_H */
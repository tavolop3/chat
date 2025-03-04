#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// --- Configuration ---
#define ARRAY_INITIAL_CAPACITY 16

// --- Allocator Interface ---
// The Allocator now includes a realloc function to simplify resizing.
typedef struct Allocator {
    // Allocate "bytes" of memory.
    void *(*alloc)(size_t bytes, void *context);
    // Free memory allocated with alloc or realloc.
    void *(*free)(size_t bytes, void *ptr, void *context);
    // Reallocate memory block to a new size.
    void *(*realloc)(void *ptr, size_t new_size, void *context);
    void *context;
} Allocator;

// Default allocation functions using the C standard library.
// The void * indicates what the function returns, while the (*alloc) indicates that alloc is a pointer to that function.
// The line (void)context; in the function my_alloc is a way to explicitly indicate that the context parameter is intentionally unused within the function.
static void *default_alloc(size_t bytes, void *context) {
    (void) context;
    return malloc(bytes);
}

static void *default_free(size_t bytes, void *ptr, void *context) {
    (void) bytes;
    (void) context;
    free(ptr);
    return NULL;
}

static void *default_realloc(void *ptr, size_t new_size, void *context) {
    (void) context;
    return realloc(ptr, new_size);
}

static Allocator default_allocator = { default_alloc, default_free, default_realloc, NULL };


// --- Dynamic Array Header ---
// The header is stored immediately before the array data.
// It stores metadata like length, capacity, and a pointer to the allocator.
typedef struct {
    size_t length;
    size_t capacity;
    size_t padding;   // prefer 16-byte alignment
    Allocator *a;
} Array_Header;

// Get the pointer to the header from the array pointer.
#define array_header(a) (((Array_Header *)(a)) - 1)

// Convenience macros to access array metadata.
#define array_length(a) (array_header(a)->length)
#define array_capacity(a) (array_header(a)->capacity)

// --- Initialization ---
// Allocates and initializes a new dynamic array for items of size "item_size" with a given capacity.
static inline void *array_init(size_t item_size, size_t capacity, Allocator *a) {
    size_t total_size = sizeof(Array_Header) + (item_size * capacity);
    Array_Header *h = (Array_Header *)a->alloc(total_size, a->context);
    if (h) {
        h->capacity = capacity;
        h->length = 0;
        h->padding = 0; // For alignment purposes
        h->a = a;
    }
    return (void *)(h + 1);
}

// Macro to create a new dynamic array for a given type using the default initial capacity.
#define array(T, allocator) ((T *)array_init(sizeof(T), ARRAY_INITIAL_CAPACITY, allocator))

// --- Ensuring Capacity ---
// Ensures there is enough room for "item_count" more items.
// It uses the allocator's realloc to adjust the size if needed.
static inline void *array_ensure_capacity(void *a, size_t item_count, size_t item_size) {
    Array_Header *h = array_header(a);
    size_t desired_capacity = h->length + item_count;
    if (h->capacity < desired_capacity) {
        size_t new_capacity = h->capacity * 2;
        while (new_capacity < desired_capacity) {
            new_capacity *= 2;
        }
        size_t new_total_size = sizeof(Array_Header) + (new_capacity * item_size);
        // Reallocate the memory block using the allocator's realloc.
        Array_Header *new_h = (Array_Header *)h->a->realloc(h, new_total_size, h->a->context);
        if (new_h) {
            new_h->capacity = new_capacity;
            a = (void *)(new_h + 1);
        } else {
            a = NULL;
        }
    }
    return a;
}

// --- Append Macro ---
// Appends a value to the dynamic array.
// It ensures enough capacity, assigns the value, increments the length, and returns a pointer to the inserted element.
#define array_append(a, v) ( \
    (a) = (typeof(a)) array_ensure_capacity((a), 1, sizeof((a)[0])), \
    (a)[ array_header(a)->length ] = (v), \
    &(a)[ array_header(a)->length++ ] \
)

// --- Removal Macros ---
// Removes the element at index i by moving the last element into its place (if not the last one).
#define array_remove(a, i) do {                               \
    Array_Header *h = array_header(a);                        \
    if ((i) < h->length) {                                    \
        if ((i) == h->length - 1) {                           \
            h->length--;                                    \
        } else {                                            \
            memcpy(&a[i], &a[h->length - 1], sizeof(a[0]));  \
            h->length--;                                    \
        }                                                   \
    }                                                       \
} while(0)

// Pops the last element from the array.
#define array_pop_back(a) do {                \
    Array_Header *h = array_header(a);        \
    if (h->length > 0) {                      \
        h->length--;                        \
    }                                         \
} while(0)

#endif // DYNAMIC_ARRAY_H

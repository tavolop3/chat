#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifndef DArray_H
#define DArray_H

// https://dylanfalconer.com/articles/dynamic-arrays-in-c

#define ARRAY_INITIAL_CAPACITY 16
#define array(T, a) array_init(sizeof(T), ARRAY_INITIAL_CAPACITY, a)
#define array_header(a) ((Array_Header *)(a) - 1)
#define array_length(a) (array_header(a)->length)
#define array_capacity(a) (array_header(a)->capacity)

typedef struct {
  void *(*alloc)(size_t bytes, void *context);
  void (*free)(void *ptr, void *context);
  void *context;
} Allocator;

typedef struct {
  size_t length;
  size_t capacity;
  size_t padding; // Prefer 16-byte alignment 
  Allocator *a;
} ArrayHeader;


void *array_init(size_t item_size, size_t capacity, Allocator *a);
void array_append(void *arr, void *item, size_t item_size);

extern Allocator my_allocator;

#endif

#include "darray.h"

/*
* The void * indicates what the function returns, while the (*alloc) indicates that alloc is a pointer to that function.
*
* The line (void)context; in the function my_alloc is a way to explicitly indicate that the context parameter is intentionally unused within the function. */

void *my_alloc(size_t bytes, void *context) {
  (void)context;
  return malloc(bytes);
}

void my_free(void *ptr, void *context) {
  (void)ptr; (void)context;
  free(ptr);
}

Allocator my_allocator = {my_alloc, my_free, 0};

void *array_init(size_t item_size, size_t capacity, Allocator *a) {
    void *ptr = 0;
    size_t size = item_size * capacity + sizeof(ArrayHeader);
    ArrayHeader *h = a->alloc(size, a->context);

    if (h) {
        h->capacity = capacity;
        h->length = 0;
        h->a = a;
        ptr = h + 1;
    }

    return ptr;
}

void array_append(void *arr, void *item, size_t item_size) {
    ArrayHeader *h = (ArrayHeader *)arr - 1;  // Obtener el encabezado
    if (h->length >= h->capacity) {
        // Redimensionar el arreglo
        size_t new_capacity = h->capacity * 2;
        size_t new_size = item_size * new_capacity + sizeof(ArrayHeader);
        ArrayHeader *new_h = h->a->alloc(new_size, h->a->context);
        if (!new_h) {
            return;  // Fallo al redimensionar
        }
        memcpy(new_h, h, item_size * h->length + sizeof(ArrayHeader));
        h->a->free(h, h->a->context);
        h = new_h;
        h->capacity = new_capacity;
        arr = (void *)(h + 1);  // Actualizar puntero al área de datos
    }
    memcpy((char *)arr + h->length * item_size, item, item_size);
    h->length++;
}

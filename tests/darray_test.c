#include <stdio.h>
#include "lib/chadarray.h"

int main(void) {
    // Create a dynamic array for integers using the default allocator.
    int *numbers = array(int, &default_allocator);

    // Append some values.
    array_append(numbers, 10);
    array_append(numbers, 20);
    array_append(numbers, 30);

    // Print the current array length and values.
    printf("Array length: %zu\n", array_length(numbers));
    for (size_t i = 0; i < array_length(numbers); i++) {
        printf("numbers[%zu] = %d\n", i, numbers[i]);
    }

    // Remove the second element (index 1).
    array_remove(numbers, 1);

    printf("After removal:\n");
    printf("Array length: %zu\n", array_length(numbers));
    for (size_t i = 0; i < array_length(numbers); i++) {
        printf("numbers[%zu] = %d\n", i, numbers[i]);
    }

    // Pop the last element.
    array_pop_back(numbers);
    printf("After pop_back:\n");
    printf("Array length: %zu\n", array_length(numbers));
    for (size_t i = 0; i < array_length(numbers); i++) {
        printf("numbers[%zu] = %d\n", i, numbers[i]);
    }

    // Free the dynamic array.
    Array_Header *header = array_header(numbers);
    size_t total_size = sizeof(Array_Header) + (header->capacity * sizeof(int));
    header->a->free(total_size, header, header->a->context);

    return 0;
}

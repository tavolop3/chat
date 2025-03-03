#include "lib/darray.h"

int main(int argc, char *argv[]) {
  int *arr = array(int, &my_allocator);
  int value1 = 69;
  int value2 = 420;
  array_append(arr, &value1, sizeof(int));
  array_append(arr, &value2, sizeof(int));

  for (size_t i = 0; i < 2; i++) {
      printf("%d\n", arr[i]);
  }

  ArrayHeader *h = (ArrayHeader *)arr - 1;
  h->a->free(h, h->a->context);

  return 0;
}

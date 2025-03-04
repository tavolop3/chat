#include <stdio.h>
#include "lib/darray.h"

int main() {
  typedef struct {
    int fd;
    char name[10];
  } User;

  User u = {1, "tavo"};
  User *a = array(User, &default_allocator);  
  array_append(a, u);

  User usuario = a[0];
  printf("%s\n", usuario.name);

  return 0;
}

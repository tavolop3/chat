#include <stdio.h>
#include <stdlib.h>

#include "darray.h"

void add_fd(int **fds, int *count, int *capacity, int fd) {
  if (*count == *capacity) {
    *capacity = (*capacity == 0) ? 10 : (*capacity * 2);
    *fds = realloc(*fds, (*capacity) * sizeof(int));
    if (!*fds) {
      perror("realloc");
      exit(EXIT_FAILURE);
    }
  }
  (*fds)[(*count)++] = fd;
  printf("agrego a la lista el fd = %d\n", fd);
}

void remove_fd(int *fds, int *count, int fd) {
  for (int i = 0; i < *count; ++i) {
    if (fds[i] == fd) {
      fds[i] = fds[--(*count)];
      break;
    }
  }
}


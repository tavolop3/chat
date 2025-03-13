#include <stdio.h>

int main() {

  for (size_t i = 0; i < strlen(usrname)+1; i++) {
    if (usrname[i] == '\0') {
      printf("usrname[%zu] = \\0\n", i);
    } else if(usrname[i] == '\n') {
      printf("usrname[%zu] = \\n\n", i);
    } else {
      printf("usrname[%zu] = %c\n", i, usrname[i]);
    }
  }
  return 0;
}

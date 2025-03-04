#include <stdio.h>
#include <string.h>

int main() {
  char usrname[10] = "tao";
  char buff[strlen(usrname)+4];

  printf("strlen(usrname)=%d\n", strlen(usrname));

  snprintf(buff, strlen(usrname)+4, "/u %s", usrname);

  printf("%s\n", buff);

  return 0;
}

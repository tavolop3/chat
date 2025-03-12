#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

int opa = 100;
void *myTurn(void *arg) {
  int *ptr = (int *)arg;
  opa = 20;
  for(int i=0; i<9; ++i) {
    sleep(1);
    printf("myturn%d:%d\n",i,*ptr);
    (*ptr)++;
  }
  return NULL;
}

void yourTurn() {
  for (int i=0; i<5; ++i) {
    sleep(1);
    printf("yourtun\n");
  }
}

int main() {
  pthread_t thread;
  int v = 5;

  pthread_create(&thread, NULL, myTurn, &v);
  
  yourTurn();

  pthread_join(thread, NULL);
  printf("final val %d\n", v);
  printf("opa%d",opa);
}

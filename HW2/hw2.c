/*
 * hw2.c
 * @author Deniz Karakay
 * @description
 * @created 2023-05-28T14:32:45.751Z+03:00
 * @last-modified 2023-05-28T15:00:08.732Z+03:00
 */

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

struct ThreadInfo {
  ucontext_t context;
  int state;
  int weight;
};

int main(int argc, char *argv[]) {
  printf("Hello World!\n");

  return 0;
}
/*
 * hw2.c
 * @author Deniz Karakay
 * @description EE442 HW2
 * @created 2023-05-28T15:01:09.513Z+03:00
 * @last-modified 2023-05-28T16:48:48.568Z+03:00
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>

#define MAX_THREADS 7
#define EMPTY 0
#define READY 1
#define RUNNING 2
#define FINISHED 3
#define STACK_SIZE 1024 * 8

struct ThreadInfo {
  ucontext_t context;
  int state;
};

struct ThreadInfo *threads[MAX_THREADS];
ucontext_t *main_uc;

void printStatus() {
  for (int i = 0; i < MAX_THREADS; i++) {
    printf("Thread %d state: %d\n", i, threads[i]->state);
  }
}

void selectThread() {
  int id = -2;
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == RUNNING) {
      id = i;
      break;
    }
  }
}

void initializeThread() {
  for (int i = 0; i < MAX_THREADS; i++) {
    threads[i] = malloc(sizeof(struct ThreadInfo));
    threads[i]->state = EMPTY;
    printf("Thread %d initialized\n", i);
  }
}

int createThread(void (*func)()) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == EMPTY) {
      ucontext_t *uc = &threads[i]->context;

      getcontext(uc);
      uc->uc_stack.ss_sp = malloc(STACK_SIZE);
      uc->uc_stack.ss_size = STACK_SIZE;
      uc->uc_link = NULL;

      if (uc->uc_stack.ss_sp == NULL) {
        perror("malloc: Could not allocate stack");
        return (1);
      }
      printf("Thread %d created\n", i);

      makecontext(uc, (void (*)(void))func, 2, 5, i);
      threads[i]->state = READY;
      return i;
    }
  }
  return -1;
}

void runThread(int signal) {
  id = selectThread();
  if (threads[id]->state == READY) {
    threads[id]->state = RUNNING;
    printf("Running thread %d\n", id);
    swapcontext(&threads[0]->context, &threads[id]->context);
  }
}

void exitThread(int id) {
  if (threads[id]->state == RUNNING) {
    printf("Exiting thread %d\n", id);
    threads[id]->state = FINISHED;
    free(threads[id]->context.uc_stack.ss_sp);
    swapcontext(&threads[id]->context, &threads[0]->context);
  }
}

void counter(int n, int i) {
  for (int val = n - i; val >= 0; val--) {
    for (int j = 0; j < i; j++) {
      printf("\t");
    }
    printf("%d\n", val);
    sleep(1);
  }
  printf("Thread %d finished\n", i);
  // exitThread(i);
}

void printThreadStates() {
  printf("running>");

  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == RUNNING) {
      printf("T%d", i);
    }
  }

  printf("\t");

  printf("ready>");
}
int main(int argc, char *argv[]) {
  initializeThread();

  signal(SIGALRM, runThread);

  for (int i = 0; i < MAX_THREADS; i++) {
    createThread(counter);
  }

  while (1) {
    sleep(3);
    raise(SIGALRM);
    printf("Raised SIGALRM for running thread\n");
  }

  return 0;
}
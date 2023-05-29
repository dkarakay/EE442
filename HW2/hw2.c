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
#define MAX_TICKETS 1000
#define EMPTY 0
#define READY 1
#define RUNNING 2
#define IO 3
#define FINISHED 4
#define STACK_SIZE 1024 * 8
#define WAIT_TIME 3

struct ThreadInfo {
  ucontext_t context;
  int state;
  int cpu_bursts[3];
  int io_bursts[3];
  int ticket_number;
  int all_bursts;
};

struct ThreadInfo *threads[MAX_THREADS];
ucontext_t main_uc;

int cpu_bursts[MAX_THREADS][3];
int io_bursts[MAX_THREADS][3];
int total_bursts;
int lottery_tickets[MAX_TICKETS];
int ticket_index;
int total_number_of_tickets;

void printStatus(int print_type) {
  if (print_type == 0) {
    printf("TID\tBursts\tState\tTickets\tCPU1\tIO1\tCPU2\tIO2\tCPU3\tIO3\n");
    for (int i = 0; i < MAX_THREADS; i++) {
      printf("T%d\t", i);
      printf("%d\t", threads[i]->all_bursts);
      printf("%d\t", threads[i]->state);
      printf("%d\t", threads[i]->ticket_number);
      for (int j = 0; j < 3; j++) {
        printf("%d\t", threads[i]->cpu_bursts[j]);
        printf("%d\t", threads[i]->io_bursts[j]);
      }

      printf("\n");
    }
    printf("\n");
  } else {
    printf("T0\tT1\tT2\tT3\tT4\tT5\tT6\n");

    int states[MAX_THREADS] = {0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < MAX_THREADS; i++) {
      states[i] = threads[i]->state;
    }

    printf("running>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == RUNNING) {
        printf("T%d", i);
      }
    }

    printf("\tready>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == READY) {
        printf("T%d ", i);
      }
    }

    printf("\tfinished>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == FINISHED) {
        printf("T%d ", i);
      }
    }

    printf("\t\tIO>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == IO) {
        printf("T%d ", i);
      }
    }
    printf("\n");
  }
}

void determineNumberOfTickets() {
  total_bursts = 0;
  for (int i = 0; i < MAX_THREADS; i++) {
    total_bursts += threads[i]->cpu_bursts[0] + threads[i]->cpu_bursts[1] +
                    threads[i]->cpu_bursts[2] + threads[i]->io_bursts[0] +
                    threads[i]->io_bursts[1] + threads[i]->io_bursts[2];
  }

  printf("Total bursts: %d\n", total_bursts);

  for (int i = 0; i < MAX_TICKETS; i++) {
    lottery_tickets[i] = -1;
  }

  ticket_index = 0;
  total_number_of_tickets = 0;
  for (int i = 0; i < MAX_THREADS; i++) {
    int ticket =
        (int)((float)(cpu_bursts[i][0] + cpu_bursts[i][1] + cpu_bursts[i][2] +
                      io_bursts[i][0] + io_bursts[i][1] + io_bursts[i][2]) /
              total_bursts * 100);
    threads[i]->ticket_number = ticket;

    for (int j = ticket_index; j < ticket_index + ticket; j++) {
      lottery_tickets[j] = i;
    }
    ticket_index += ticket;
    total_number_of_tickets += ticket;
  }
}

void PWFScheduler() {
  // Pick a random number in ticket index
  int random_number = -1;
  int state = IO;
  int selected_thread = -1;

  while (random_number == -1 && state == IO) {
    random_number = rand() % ticket_index;
    selected_thread = lottery_tickets[random_number];
    state = threads[selected_thread]->state;
  }

  if (threads[selected_thread]->state == READY) {
    threads[selected_thread]->state = RUNNING;
    threads[selected_thread]->ticket_number -= 1;
    total_number_of_tickets--;
  }
  printf("Running thread: %d\n", selected_thread);
  // printStatus(0);
  printStatus(1);

  int all_bursts = threads[selected_thread]->all_bursts;
  int cpu1_current = threads[selected_thread]->cpu_bursts[0];
  int cpu2_current = threads[selected_thread]->cpu_bursts[1];
  int cpu3_current = threads[selected_thread]->cpu_bursts[2];

  int io1_current = threads[selected_thread]->io_bursts[0];
  int io2_current = threads[selected_thread]->io_bursts[1];
  int io3_current = threads[selected_thread]->io_bursts[2];

  int cpu1 = cpu_bursts[selected_thread][0];
  int cpu2 = cpu_bursts[selected_thread][1];
  int cpu3 = cpu_bursts[selected_thread][2];

  int io1 = io_bursts[selected_thread][0];
  int io2 = io_bursts[selected_thread][1];
  int io3 = io_bursts[selected_thread][2];

  int first = cpu_bursts[selected_thread][0] + io_bursts[selected_thread][0];
  int second = cpu_bursts[selected_thread][1] + io_bursts[selected_thread][1];
  int third = cpu_bursts[selected_thread][2] + io_bursts[selected_thread][2];
  int wait = 0;

  if (all_bursts == cpu1) {
  }

  else if (all_bursts < cpu1) {
    wait = cpu1 - all_bursts;
    if (wait > WAIT_TIME) {
      threads[selected_thread]->all_bursts += WAIT_TIME;
      threads[selected_thread]->cpu_bursts[0] -= WAIT_TIME;
      threads[selected_thread]->state = READY;
    } else {
      threads[selected_thread]->all_bursts += wait;
      threads[selected_thread]->cpu_bursts[0] -= wait;
      threads[selected_thread]->state = IO;
    }

  }

  else if (all_bursts < first + cpu2) {
    wait = first + cpu2 - all_bursts;
    if (wait > WAIT_TIME) {
      threads[selected_thread]->all_bursts += WAIT_TIME;
      threads[selected_thread]->cpu_bursts[1] -= WAIT_TIME;
    } else {
      threads[selected_thread]->all_bursts += wait;
      threads[selected_thread]->cpu_bursts[1] -= wait;
    }
    threads[selected_thread]->state = READY;
  }

  int check = 0;
  if (wait > WAIT_TIME) {
    check = wait - WAIT_TIME;
  }

  printf("Thread: %d Wait: %d\n", selected_thread, wait);
  for (int val = wait; val > check; val--) {
    for (int j = 0; j < selected_thread; j++) {
      printf("\t");
    }
    printf("%d\n", val);
  }
}

int selectThread() {
  int id = -2;
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state != FINISHED) {
      id = i;
      return id;
    }
  }
}

void initializeThread() {
  for (int i = 0; i < MAX_THREADS; i++) {
    threads[i] = malloc(sizeof(struct ThreadInfo));
    threads[i]->state = EMPTY;

    threads[i]->cpu_bursts[0] = cpu_bursts[i][0];
    threads[i]->cpu_bursts[1] = cpu_bursts[i][1];
    threads[i]->cpu_bursts[2] = cpu_bursts[i][2];

    threads[i]->io_bursts[0] = io_bursts[i][0];
    threads[i]->io_bursts[1] = io_bursts[i][1];
    threads[i]->io_bursts[2] = io_bursts[i][2];

    threads[i]->all_bursts = 0;
  }
}

int createThread() {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == EMPTY) {
      ucontext_t *uc = &threads[i]->context;

      getcontext(uc);
      uc->uc_stack.ss_sp = malloc(STACK_SIZE);
      uc->uc_stack.ss_size = STACK_SIZE;
      uc->uc_link = &main_uc;

      if (uc->uc_stack.ss_sp == NULL) {
        perror("malloc: Could not allocate stack");
        return (1);
      }

      makecontext(uc, (void (*)(void))PWFScheduler, 1);
      threads[i]->state = READY;
      return i;
    }
  }
  return -1;
}

void runThread(int signal) {
  int id = selectThread();
  ucontext_t *uc = &threads[id]->context;

  getcontext(uc);

  makecontext(uc, (void (*)(void))PWFScheduler, 1);
  swapcontext(&main_uc, &threads[id]->context);
}

void exitThread(int id) {
  if (threads[id]->state == RUNNING) {
    printf("Exiting thread %d\n", id);
    threads[id]->state = FINISHED;
    free(threads[id]->context.uc_stack.ss_sp);
    swapcontext(&threads[id]->context, &threads[0]->context);
  }
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

void printInputData() {
  printf("TID\tCPU1\tCPU2\tCPU3\tIO1\tIO2\tIO3\n");
  for (int i = 0; i < MAX_THREADS; i++) {
    printf("T%d\t", i);
    for (int j = 0; j < 3; j++) {
      printf("%d\t", cpu_bursts[i][j]);
    }
    for (int j = 0; j < 3; j++) {
      printf("%d\t", io_bursts[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

void readInputFromTxt() {
  FILE *fp;
  char filename[] = "input.txt";
  int cpu1, cpu2, cpu3, io1, io2, io3;

  fp = fopen(filename, "r");

  if (fp == NULL) {
    perror("Error while opening the file.\n");
    exit(EXIT_FAILURE);
  }

  int thread_number = 0;
  while (fscanf(fp, "%d %d %d %d %d %d", &cpu1, &cpu2, &cpu3, &io1, &io2,
                &io3) != EOF) {
    cpu_bursts[thread_number][0] = cpu1;
    cpu_bursts[thread_number][1] = cpu2;
    cpu_bursts[thread_number][2] = cpu3;

    io_bursts[thread_number][0] = io1;
    io_bursts[thread_number][1] = io2;
    io_bursts[thread_number][2] = io3;

    thread_number++;
  }

  fclose(fp);
}

int main(int argc, char *argv[]) {
  readInputFromTxt();
  printInputData();

  getcontext(&main_uc);

  initializeThread();

  signal(SIGALRM, runThread);
  determineNumberOfTickets();

  for (int i = 0; i < MAX_THREADS; i++) {
    createThread();
  }

  printStatus(0);

  while (1) {
    sleep(3);
    raise(SIGALRM);
  }
  return 0;
}
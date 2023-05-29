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
int all_finished;

void exitThread(int id);

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
  } else if (print_type == 1) {
    // printf("\nT0\tT1\tT2\tT3\tT4\tT5\tT6\n");

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

    int ready = 0;
    int first_printed = 0;
    printf("\tready>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == READY) {
        ready++;
        if (first_printed) {
          printf(",");
        }
        printf("T%d", i);
        first_printed = 1;
      }
    }

    for (int i = 0; i < MAX_THREADS - ready; i++) {
      printf("   ");
    }

    int finished = 0;
    first_printed = 0;
    printf("\tfinished>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == FINISHED) {
        finished++;
        if (first_printed) {
          printf(",");
        }
        printf("T%d", i);
        first_printed = 1;
      }
    }

    for (int i = 0; i < MAX_THREADS - finished; i++) {
      printf("   ");
    }

    printf("\t\tIO>");
    for (int i = 0; i < MAX_THREADS; i++) {
      if (states[i] == IO) {
        printf("T%d ", i);
      }
    }
    printf("\n");
  } else if (print_type == 2) {
    printf("T0\tT1\tT2\tT3\tT4\tT5\tT6\n");
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
  printf("\n");

  for (int i = 0; i < MAX_TICKETS; i++) {
    lottery_tickets[i] = -1;
  }

  ticket_index = 0;
  total_number_of_tickets = 0;
  for (int i = 0; i < MAX_THREADS; i++) {
    int ticket = (cpu_bursts[i][0] + cpu_bursts[i][1] + cpu_bursts[i][2] +
                  io_bursts[i][0] + io_bursts[i][1] + io_bursts[i][2]);
    threads[i]->ticket_number = ticket;

    for (int j = ticket_index; j < ticket_index + ticket; j++) {
      lottery_tickets[j] = i;
    }
    ticket_index += ticket;
    total_number_of_tickets += ticket;
  }
}

void checkIO(int wait) {
  if (wait > WAIT_TIME) {
    wait = WAIT_TIME;
  }

  // Check IO
  for (int i = 0; i < MAX_THREADS; i++) {
    int temp_state = threads[i]->state;

    // If thread is finished
    if (temp_state != IO) {
      continue;
    }

    int temp_all_bursts = threads[i]->all_bursts;
    int temp_cpu1 = cpu_bursts[i][0];
    int temp_cpu2 = cpu_bursts[i][1];
    int temp_cpu3 = cpu_bursts[i][2];

    int temp_io1 = io_bursts[i][0];
    int temp_io2 = io_bursts[i][1];
    int temp_io3 = io_bursts[i][2];

    int current_io1 = threads[i]->io_bursts[0];
    int current_io2 = threads[i]->io_bursts[1];
    int current_io3 = threads[i]->io_bursts[2];

    int temp_first = temp_cpu1 + temp_io1;
    int temp_second = temp_cpu2 + temp_io2;
    int temp_third = temp_cpu3 + temp_io3;

    int temp_wait = 0;
    int temp_check = 0;
    int temp_from = 0;

    // If thread is in IO state and we are in the first IO burst
    if (temp_all_bursts < temp_first) {
      if (current_io1 > wait) {
        threads[i]->io_bursts[0] -= wait;
        temp_wait = wait;
        temp_check = 1;
      } else {
        threads[i]->io_bursts[0] -= current_io1;
        temp_wait = current_io1;
        temp_check = 2;
      }
      temp_from = 1;

    }
    // If thread is in IO state and we are in the second IO burst
    else if (temp_all_bursts < temp_first + temp_second) {
      if (current_io2 > wait) {
        threads[i]->io_bursts[1] -= wait;
        temp_wait = wait;
        temp_check = 1;
      } else {
        threads[i]->io_bursts[1] -= current_io2;
        temp_wait = current_io2;
        temp_check = 2;
      }
      temp_from = 2;

    }

    // If thread is in IO state and we are in the third IO burst
    else if (temp_all_bursts < temp_first + temp_second + temp_third) {
      if (current_io3 > wait) {
        threads[i]->io_bursts[2] -= wait;
        temp_wait = wait;
        temp_check = 1;
      } else {
        threads[i]->io_bursts[2] -= current_io3;
        temp_wait = current_io3;
        temp_check = 2;
      }
      temp_from = 3;
    }

    if (temp_check != 0) {
      threads[i]->all_bursts += temp_wait;
      threads[i]->ticket_number -= temp_wait;

      if (temp_check == 1) {
        threads[i]->state = IO;
        break;
      } else if (temp_check == 2) {
        threads[i]->state = READY;
        if (temp_from == 3) {
          exitThread(i);
          return;
        }

        if (wait == 0) {
          break;
        }
        wait -= temp_wait;
      }
    }
  }
}

// Print step of process and wait for 1 second
void printStep(int wait, int selected_thread, int check) {
  for (int val = wait; val > check; val--) {
    for (int j = 0; j < selected_thread; j++) {
      printf("\t");
    }
    printf("%d\n", val - 1);
    sleep(0);
  }
}

void PWFScheduler() {
  // Check if all threads are finished
  all_finished = 0;
  int all_io = 0;
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == FINISHED) {
      all_finished++;
    }

    if (threads[i]->state == IO) {
      all_io++;
    }
  }

  // Pick a random number in ticket index
  int random_number = rand() % ticket_index;
  int selected_thread = lottery_tickets[random_number];
  int state = threads[selected_thread]->state;

  if (all_io == 2 && all_finished == MAX_THREADS - 2) {
    checkIO(WAIT_TIME);

  }

  else if (all_finished == MAX_THREADS - 1 && all_io == 1) {
    // Last thread is in IO
    for (int i = 0; i < MAX_THREADS; i++) {
      if (threads[i]->state == READY) {
        selected_thread = i;
        state = threads[i]->state;
        return;
      }

      if (threads[i]->state == IO) {
        exitThread(i);
        return;
      }
    }
  } else {
    while (state != READY) {
      selected_thread += 1;
      selected_thread %= MAX_THREADS;
      state = threads[selected_thread]->state;
    }
  }

  if (threads[selected_thread]->state == READY) {
    threads[selected_thread]->state = RUNNING;
    total_number_of_tickets--;
  }

  // printStatus(0);
  printStatus(1);

  int all_bursts = threads[selected_thread]->all_bursts;

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

  // If we are in the first CPU burst
  if (all_bursts < cpu1) {
    wait = cpu1 - all_bursts;
    if (wait > WAIT_TIME) {
      threads[selected_thread]->cpu_bursts[0] -= WAIT_TIME;
    } else {
      threads[selected_thread]->cpu_bursts[0] -= wait;
    }

  }

  // If we are in the second CPU burst
  else if (all_bursts < first + cpu2) {
    wait = first + cpu2 - all_bursts;
    if (wait > WAIT_TIME) {
      threads[selected_thread]->cpu_bursts[1] -= WAIT_TIME;
    } else {
      threads[selected_thread]->cpu_bursts[1] -= wait;
    }
  }

  // If we are in the third CPU burst
  else if (all_bursts < first + second + cpu3) {
    wait = first + second + cpu3 - all_bursts;
    if (wait > WAIT_TIME) {
      threads[selected_thread]->cpu_bursts[2] -= WAIT_TIME;
    } else {
      threads[selected_thread]->cpu_bursts[2] -= wait;
    }
  }

  checkIO(wait);

  int check = 0;
  if (wait > WAIT_TIME) {
    threads[selected_thread]->all_bursts += WAIT_TIME;
    threads[selected_thread]->ticket_number -= WAIT_TIME;
    threads[selected_thread]->state = READY;
    check = wait - WAIT_TIME;
  } else if (wait <= WAIT_TIME && threads[selected_thread]->state == RUNNING) {
    threads[selected_thread]->all_bursts += wait;
    threads[selected_thread]->ticket_number -= wait;
    threads[selected_thread]->state = IO;
    check = 0;
  }

  printStep(wait, selected_thread, check);
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

// Initialize threads
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

// Create thread
int createThread() {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == EMPTY) {
      // Get context
      ucontext_t *uc = &threads[i]->context;

      getcontext(uc);

      // Set context values
      uc->uc_stack.ss_sp = malloc(STACK_SIZE);
      uc->uc_stack.ss_size = STACK_SIZE;

      // Set context link to main context
      uc->uc_link = &main_uc;

      if (uc->uc_stack.ss_sp == NULL) {
        perror("malloc: Could not allocate stack");
        return (1);
      }

      // Make context and set function to run
      makecontext(uc, (void (*)(void))PWFScheduler, 1);

      // Set state to READY
      threads[i]->state = READY;
      return i;
    }
  }
  return -1;
}

// Run thread
void runThread(int signal) {
  // Pick proper thread
  int id = selectThread();

  // Get context
  ucontext_t *uc = &threads[id]->context;

  getcontext(uc);

  // Set context values
  makecontext(uc, (void (*)(void))PWFScheduler, 1);

  // Swap context
  swapcontext(&main_uc, &threads[id]->context);
}

// Exit thread
void exitThread(int id) {
  // If we reached the last thread
  if (all_finished == MAX_THREADS - 1) {
    // Increase all bursts
    threads[id]->all_bursts += threads[id]->io_bursts[2];

    // Decrease Ticket
    threads[id]->ticket_number -= threads[id]->io_bursts[2];

    // Decrease IO burst
    threads[id]->io_bursts[2] -= threads[id]->io_bursts[2];

    // Finalize remaining thread
    threads[id]->state = FINISHED;

    // Increase all finished
    all_finished++;

    // printStatus(0);
    printStatus(1);

    // If we are not in the last thread
  } else {
    // Change state to FINISHED
    threads[id]->state = FINISHED;
  }

  // Free stack
  free(threads[id]->context.uc_stack.ss_sp);
}

// Print input data
void printInputData() {
  // Print CPU and IO bursts for each thread
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

// Read input data from txt file
void readInputFromTxt() {
  FILE *fp;

  // File name
  char filename[] = "input.txt";
  int cpu1, cpu2, cpu3, io1, io2, io3;

  // Open file
  fp = fopen(filename, "r");

  // If file does not exist
  if (fp == NULL) {
    perror("Error while opening the file.\n");
    exit(EXIT_FAILURE);
  }

  // Read input data
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

  // Close file
  fclose(fp);
}

int main(int argc) {
  // Read input data from txt file
  readInputFromTxt();

  // Print input data
  printInputData();

  // Get main context
  getcontext(&main_uc);

  // Initialize threads
  initializeThread();

  // Set signal handler
  signal(SIGALRM, runThread);

  // Determine number of tickets initially
  determineNumberOfTickets();

  // Create threads
  for (int i = 0; i < MAX_THREADS; i++) {
    createThread();
  }

  // printStatus(0);
  printStatus(2);
  all_finished = 0;
  while (all_finished != MAX_THREADS) {
    raise(SIGALRM);
  }

  return 0;
}
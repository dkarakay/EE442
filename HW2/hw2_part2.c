/*
 * Created on Wed May 31 2023
 * Deniz Karakay - 2443307
 * EE442 - HW2 - Part 2
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
#define MAX_NUMBER 100000

struct ThreadInfo {
  ucontext_t context;
  int state;
  int cpu_bursts[3];
  int io_bursts[3];
  int remaining;
  int all_bursts;
};

struct ThreadInfo *threads[MAX_THREADS];
ucontext_t main_uc;

int cpu_bursts[MAX_THREADS][3];
int io_bursts[MAX_THREADS][3];
int total_bursts;
int all_finished;

void exitThread(int id);

// Print status of threads
void printStatus(int print_type) {
  if (print_type == 0) {
    printf("TID\tBursts\tState\tRemaining\tCPU1\tIO1\tCPU2\tIO2\tCPU3\tIO3\n");
    for (int i = 0; i < MAX_THREADS; i++) {
      printf("T%d\t", i);
      printf("%d\t", threads[i]->all_bursts);
      printf("%d\t", threads[i]->state);
      printf("%d\t\t", threads[i]->remaining);
      for (int j = 0; j < 3; j++) {
        printf("%d\t", threads[i]->cpu_bursts[j]);
        printf("%d\t", threads[i]->io_bursts[j]);
      }

      printf("\n");
    }
    printf("\n");
  } else if (print_type == 1) {
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

// Determine number of tickets initially for lottery scheduling
void determineRemainingBursts() {
  total_bursts = 0;
  for (int i = 0; i < MAX_THREADS; i++) {
    total_bursts += threads[i]->cpu_bursts[0] + threads[i]->cpu_bursts[1] +
                    threads[i]->cpu_bursts[2] + threads[i]->io_bursts[0] +
                    threads[i]->io_bursts[1] + threads[i]->io_bursts[2];
  }

  printf("Total bursts: %d\n", total_bursts);
  printf("\n");

  // Determine number of tickets for each thread
  for (int i = 0; i < MAX_THREADS; i++) {
    int remaining = 0;
    if (cpu_bursts[i][0] != 0) {
      remaining = cpu_bursts[i][0];
    } else if (cpu_bursts[i][1] != 0) {
      remaining = cpu_bursts[i][1];
    } else if (cpu_bursts[i][2] != 0) {
      remaining = cpu_bursts[i][2];
    }
    threads[i]->remaining = remaining;
  }
}

// Check IO bursts
void checkIO() {
  int wait = 1;
  // Check IO
  for (int i = 0; i < MAX_THREADS; i++) {
    int temp_state = threads[i]->state;
    // If thread is finished
    if (temp_state != IO) {
      continue;
    }

    // Temp variables
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

    int temp_check = 0;
    int temp_from = 0;

    // If thread is in IO state and we are in the first IO burst
    if (temp_all_bursts < temp_first) {
      // If we are in the first IO burst and we do not finish IO
      threads[i]->io_bursts[0] -= wait;
      if (threads[i]->io_bursts[0] != 0) {
        temp_check = 1;
        // If we are in the first IO burst and we will finish IO
      } else {
        temp_check = 2;
      }
      temp_from = 1;

    }
    // If thread is in IO state and we are in the second IO burst
    else if (temp_all_bursts < temp_first + temp_second) {
      threads[i]->io_bursts[1] -= wait;
      // If we are in the second IO burst and we have to wait more than 3
      if (threads[i]->io_bursts[1] != 0) {
        temp_check = 1;
        // If we are in the second IO burst and we will finish IO
      } else {
        temp_check = 2;
      }
      temp_from = 2;

    }

    // If thread is in IO state and we are in the third IO burst
    else if (temp_all_bursts < temp_first + temp_second + temp_third) {
      threads[i]->io_bursts[2] -= wait;
      // If we are in the third IO burst and we have to wait more than 3
      if (threads[i]->io_bursts[2] != 0) {
        temp_check = 1;
        // If we are in the third IO burst and we will finish IO
      } else {
        temp_check = 2;
      }
      temp_from = 3;
    }

    if (temp_check != 0) {
      // Increase all bursts and decrease ticket number
      threads[i]->all_bursts += wait;

      // We need to wait more
      if (temp_check == 1) {
        threads[i]->state = IO;
        break;
        // We finished IO
      } else if (temp_check == 2) {
        threads[i]->state = READY;

        if (temp_from == 1) {
          threads[i]->remaining = temp_cpu2;
        } else if (temp_from == 2) {
          threads[i]->remaining = temp_cpu3;
        }
        // If we are in the last IO burst
        if (temp_from == 3) {
          // Exit thread
          exitThread(i);
          return;
        }
      }
    }
  }
}

// Print step of process and wait for 1 second
void printStep(int selected_thread, int val) {
  for (int j = 0; j < selected_thread; j++) {
    printf("\t");
  }
  printf("%d\n", val);
  sleep(0);
}

// Shortest remaining time first scheduler
void SRTFScheduler() {
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

  if (all_io + all_finished == MAX_THREADS) {
    checkIO();
    return;
  }

  // Check the minimum remaining burst
  int min_remaining = 100000;
  int min_index = -1;
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads[i]->state == READY) {
      if (threads[i]->remaining < min_remaining) {
        min_remaining = threads[i]->remaining;
        min_index = i;
      }
    }
  }
  int selected_thread = min_index;

  // Get running indexes
  for (int t = 0; t < WAIT_TIME; t++) {
    /*printf("Selected thread: %d status: %d\n", selected_thread,
           threads[selected_thread]->state);*/
    if (threads[selected_thread]->state == IO) {
      break;
    }
    // If there is no thread to run
    /*if (min_index == -1) {
      checkIO(WAIT_TIME);
      return;
    }*/

    if (threads[selected_thread]->state == READY) {
      threads[selected_thread]->state = RUNNING;
    }

    // Print status

    if (t % 3 == 0) {
      printStatus(0);
      printStatus(1);
    }

    // Variables
    int all_bursts = threads[selected_thread]->all_bursts;
    int remaining = threads[selected_thread]->remaining;

    int cpu1 = cpu_bursts[selected_thread][0];
    int cpu2 = cpu_bursts[selected_thread][1];
    int cpu3 = cpu_bursts[selected_thread][2];

    int io1 = io_bursts[selected_thread][0];
    int io2 = io_bursts[selected_thread][1];
    int io3 = io_bursts[selected_thread][2];

    int first = cpu_bursts[selected_thread][0] + io_bursts[selected_thread][0];
    int second = cpu_bursts[selected_thread][1] + io_bursts[selected_thread][1];
    int third = cpu_bursts[selected_thread][2] + io_bursts[selected_thread][2];
    int wait = 1;
    int val = -1;

    // If we are in the first CPU burst
    if (all_bursts < cpu1) {
      threads[selected_thread]->cpu_bursts[0] -= wait;

      if (threads[selected_thread]->cpu_bursts[0] == 0) {
        threads[selected_thread]->state = IO;
        threads[selected_thread]->remaining = MAX_NUMBER;
      } else {
        threads[selected_thread]->state = READY;
        threads[selected_thread]->remaining -= wait;
      }

      val = threads[selected_thread]->cpu_bursts[0];

    }

    // If we are in the second CPU burst
    else if (all_bursts < first + cpu2) {
      threads[selected_thread]->cpu_bursts[1] -= wait;
      if (threads[selected_thread]->cpu_bursts[1] == 0) {
        threads[selected_thread]->state = IO;
        threads[selected_thread]->remaining = MAX_NUMBER;
      } else {
        threads[selected_thread]->state = READY;
        threads[selected_thread]->remaining -= wait;
      }

      val = threads[selected_thread]->cpu_bursts[1];

    }

    // If we are in the third CPU burst
    else if (all_bursts < first + second + cpu3) {
      threads[selected_thread]->cpu_bursts[2] -= wait;
      if (threads[selected_thread]->cpu_bursts[2] == 0) {
        threads[selected_thread]->state = IO;
        threads[selected_thread]->remaining = MAX_NUMBER;

      } else {
        threads[selected_thread]->state = READY;
        threads[selected_thread]->remaining -= wait;
      }

      val = threads[selected_thread]->cpu_bursts[2];
    }

    if (val != -1) {
      // Print steps and wait for 1 second
      printStep(selected_thread, val);
    }
    // Check IO
    checkIO();

    threads[selected_thread]->all_bursts += wait;
  }
}

// Select suitable thread
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
    threads[i]->remaining = 0;
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
      makecontext(uc, (void (*)(void))SRTFScheduler, 1);

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
  makecontext(uc, (void (*)(void))SRTFScheduler, 1);

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
    threads[id]->remaining -= threads[id]->io_bursts[2];

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
// Details and structure of input.txt file is in the input_format.txt file
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
  determineRemainingBursts();

  // Create threads
  for (int i = 0; i < MAX_THREADS; i++) {
    createThread();
  }

  printStatus(2);

  // Set all finished to 0
  all_finished = 0;

  // Set alarm till all threads are finished
  while (all_finished != MAX_THREADS) {
    raise(SIGALRM);
  }

  /*
  // Free main context
  free(main_uc.uc_stack.ss_sp);
  free(main_uc.uc_link);

  // Free threads
  for (int i = 0; i < MAX_THREADS; i++) {
    free(threads[i]);
  }*/

  return 0;
}
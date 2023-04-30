// Created by: Deniz Karakay
// Created on: 29 April 2023
// Last modified on: 30 April 2023

#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_ATOM_TYPES 5

// Struct to hold atom
struct atom {
  int atomID;
  char atomTYPE;  // C, N, S, O or TH
};

// Default values
int number_m = 60;
int rate_g = 100;

char compose_molecule[5] = "";

// Atom types
char atom_types[NUM_ATOM_TYPES] = {'C', 'N', 'S', 'T', 'O'};
int current_atoms_count[NUM_ATOM_TYPES] = {0, 0, 0, 0, 0};

// Molecule order
char molecule_order[5] = {'C', 'N', 'C', 'S', 'T'};
int molecule_order_index = 0;

struct atom *atoms_c, *atoms_n, *atoms_s, *atoms_th, *atoms_o;

sem_t sem_produce_c, sem_produce_n, sem_produce_s, sem_produce_th,
    sem_produce_o;

// Custom sleep function
void sleep_func() {
  double sleep_time = -log(1 - (double)rand() / RAND_MAX) / rate_g;
  usleep(sleep_time * 1000000);
}

void *Produce_C(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    sem_post(&sem_produce_c);
    struct atom carbon_atom;
    carbon_atom.atomID = i + 1;
    carbon_atom.atomTYPE = 'C';
    atoms_c[i] = carbon_atom;
    printf("C with ID: %d is created.\n", i + 1);
    sleep_func();
  }
  return NULL;
}

void *Produce_N(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    sem_post(&sem_produce_n);
    struct atom nitrogen_atom;
    nitrogen_atom.atomID = i + 1;
    nitrogen_atom.atomTYPE = 'N';
    atoms_n[i] = nitrogen_atom;
    printf("N with ID: %d is created.\n", i + 1);
    sleep_func();
  }
  return NULL;
}

void *Produce_S(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    sem_post(&sem_produce_s);
    struct atom sulfur_atom;
    sulfur_atom.atomID = i + 1;
    sulfur_atom.atomTYPE = 'S';
    atoms_s[i] = sulfur_atom;
    printf("S with ID: %d is created.\n", i + 1);
    sleep_func();
  }
  return NULL;
}

void *Produce_Th(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    sem_post(&sem_produce_th);
    struct atom thorium_atom;
    thorium_atom.atomID = i + 1;
    thorium_atom.atomTYPE = 'T';
    atoms_th[i] = thorium_atom;
    printf("T with ID: %d is created.\n", i + 1);
    sleep_func();
  }
  return NULL;
}

void *Produce_O(void *arg) {
  int limit = number_m / 3;
  for (int i = 0; i < limit; i++) {
    sem_post(&sem_produce_o);
    struct atom oxygen_atom;
    oxygen_atom.atomID = i + 1;
    oxygen_atom.atomTYPE = 'O';
    atoms_o[i] = oxygen_atom;
    printf("O with ID: %d is created.\n", i + 1);
    sleep_func();
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int opt;

  unsigned int seed = 42;
  srand(seed);

  // Parse command line arguments
  while ((opt = getopt(argc, argv, "m:g:")) != -1) {
    switch (opt) {
      case 'm':
        number_m = atoi(optarg);
        break;
      case 'g':
        rate_g = atoi(optarg);
        break;
      default:
        break;
    }
  }
  // Print counts and rate
  printf("Number of molecules: %d and Rate of generation: %d\n", number_m,
         rate_g);

  atoms_c = malloc(number_m / 6 * sizeof(struct atom));
  atoms_n = malloc(number_m / 6 * sizeof(struct atom));
  atoms_s = malloc(number_m / 6 * sizeof(struct atom));
  atoms_th = malloc(number_m / 6 * sizeof(struct atom));
  atoms_o = malloc(number_m / 3 * sizeof(struct atom));

  sem_init(&sem_produce_c, 0, 0);
  sem_init(&sem_produce_n, 0, 0);
  sem_init(&sem_produce_s, 0, 0);
  sem_init(&sem_produce_th, 0, 0);
  sem_init(&sem_produce_o, 0, 0);

  pthread_t t_produce_c, t_produce_n, t_produce_s, t_produce_th, t_produce_o;
  pthread_create(&t_produce_c, NULL, Produce_C, NULL);
  pthread_create(&t_produce_n, NULL, Produce_N, NULL);
  pthread_create(&t_produce_s, NULL, Produce_S, NULL);
  pthread_create(&t_produce_th, NULL, Produce_Th, NULL);
  pthread_create(&t_produce_o, NULL, Produce_O, NULL);

  pthread_join(t_produce_c, NULL);
  pthread_join(t_produce_n, NULL);
  pthread_join(t_produce_s, NULL);
  pthread_join(t_produce_th, NULL);
  pthread_join(t_produce_o, NULL);

  return 0;
}
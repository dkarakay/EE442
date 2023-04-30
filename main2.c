// Created by: Deniz Karakay
// Created on: 29 April 2023
// Last modified on: 30 April 2023

#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Atom ID
int atomID = 1;

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

sem_t sem_compose_co2, sem_compose_no2, sem_compose_so2, sem_compose_tho2,
    sem_print_molecule_type;

int order = 0;

// Custom sleep function
void sleep_func() {
  double sleep_time = -log(1 - (double)rand() / RAND_MAX) / rate_g;
  usleep(sleep_time * 1000000);
}

void *Produce_C(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    // Post semaphore to produce C
    sem_post(&sem_produce_c);

    // Create C atom
    struct atom carbon_atom;
    carbon_atom.atomID = atomID++;
    carbon_atom.atomTYPE = 'C';

    // Add C atom to atoms_c array
    atoms_c[i] = carbon_atom;
    current_atoms_count[0]++;

    // Print C atom
    printf("%c with ID: %d is created.\n", carbon_atom.atomTYPE,
           carbon_atom.atomID);

    // Sleep
    sleep_func();
  }
  return NULL;
}

void *Produce_N(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    // Post semaphore to produce N
    sem_post(&sem_produce_n);

    // Create N atom
    struct atom nitrogen_atom;
    nitrogen_atom.atomID = atomID++;
    nitrogen_atom.atomTYPE = 'N';

    // Add N atom to atoms_n array
    atoms_n[i] = nitrogen_atom;
    current_atoms_count[1]++;

    // Print N atom
    printf("%c with ID: %d is created.\n", nitrogen_atom.atomTYPE,
           nitrogen_atom.atomID);

    // Sleep
    sleep_func();
  }
  return NULL;
}

void *Produce_S(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    // Post semaphore to produce S
    sem_post(&sem_produce_s);

    // Create S atom
    struct atom sulfur_atom;
    sulfur_atom.atomID = atomID++;
    sulfur_atom.atomTYPE = 'S';

    // Add S atom to atoms_s array
    atoms_s[i] = sulfur_atom;
    current_atoms_count[2]++;

    // Print S atom
    printf("%c with ID: %d is created.\n", sulfur_atom.atomTYPE,
           sulfur_atom.atomID);

    // Sleep
    sleep_func();
  }
  return NULL;
}

void *Produce_TH(void *arg) {
  int limit = number_m / 6;
  for (int i = 0; i < limit; i++) {
    // Post semaphore to produce TH
    sem_post(&sem_produce_th);

    // Create TH atom
    struct atom thorium_atom;
    thorium_atom.atomID = atomID++;
    thorium_atom.atomTYPE = 'T';

    // Add TH atom to atoms_th array
    atoms_th[i] = thorium_atom;
    current_atoms_count[3]++;

    // Print TH atom
    printf("%c with ID: %d is created.\n", thorium_atom.atomTYPE,
           thorium_atom.atomID);

    // Sleep
    sleep_func();
  }
  return NULL;
}

void *Produce_O(void *arg) {
  int limit = number_m / 3;
  for (int i = 0; i < limit; i++) {
    // Post semaphore to produce O
    sem_post(&sem_produce_o);

    // Create O atom
    struct atom oxygen_atom;
    oxygen_atom.atomID = atomID++;
    oxygen_atom.atomTYPE = 'O';

    // Add O atom to atoms_o array
    atoms_o[i] = oxygen_atom;
    current_atoms_count[4]++;

    // Print O atom
    printf("%c with ID: %d is created.\n", oxygen_atom.atomTYPE,
           oxygen_atom.atomID);

    // Sleep
    sleep_func();
  }
  return NULL;
}

void *Composer_CO2(void *arg) {
  while (1) {
    // Wait for CO2 semaphore to be posted
    sem_wait(&sem_compose_co2);

    // Wait for 1 C and 2 O atoms to be produced
    sem_wait(&sem_produce_c);
    sem_wait(&sem_produce_o);
    sem_wait(&sem_produce_o);

    // Create CO2 molecule
    current_atoms_count[0]--;
    current_atoms_count[4] -= 2;

    // Copy CO2 to compose_molecule char array
    strcpy(compose_molecule, "CO2");

    // Print CO2 molecule
    sem_post(&sem_print_molecule_type);

    // Change order of molecule composition
    // If order is 0, next molecule is NO2 or if it is 1, next molecule is SO2
    if (order == 0) {
      sem_post(&sem_compose_no2);
      order = 1;

    } else if (order == 1) {
      sem_post(&sem_compose_so2);
      order = 0;
    }
  }
}

void *Composer_NO2(void *arg) {
  while (1) {
    // Wait for NO2 semaphore to be posted
    sem_wait(&sem_compose_no2);

    // Wait for 1 N and 2 O atoms to be produced
    sem_wait(&sem_produce_n);
    sem_wait(&sem_produce_o);
    sem_wait(&sem_produce_o);

    // Create NO2 molecule
    current_atoms_count[1]--;
    current_atoms_count[4] -= 2;

    // Copy NO2 to compose_molecule char array
    strcpy(compose_molecule, "NO2");

    // Print NO2 molecule
    sem_post(&sem_print_molecule_type);

    // Change order of molecule composition
    sem_post(&sem_compose_co2);
  }
}

void *Composer_SO2(void *arg) {
  while (1) {
    // Wait for SO2 semaphore to be posted
    sem_wait(&sem_compose_so2);

    // Wait for 1 S and 2 O atoms to be produced
    sem_wait(&sem_produce_s);
    sem_wait(&sem_produce_o);
    sem_wait(&sem_produce_o);

    // Create SO2 molecule
    current_atoms_count[2]--;
    current_atoms_count[4] -= 2;

    // Copy SO2 to compose_molecule char array
    strcpy(compose_molecule, "SO2");

    // Print SO2 molecule
    sem_post(&sem_print_molecule_type);

    // Change order of molecule composition
    sem_post(&sem_compose_tho2);
  }
}

void *Composer_ThO2(void *arg) {
  while (1) {
    // Wait for ThO2 semaphore to be posted
    sem_wait(&sem_compose_tho2);

    // Wait for 1 Th and 2 O atoms to be produced
    sem_wait(&sem_produce_th);
    sem_wait(&sem_produce_o);
    sem_wait(&sem_produce_o);

    // Create ThO2 molecule
    current_atoms_count[3]--;
    current_atoms_count[4] -= 2;

    // Copy ThO2 to compose_molecule char array
    strcpy(compose_molecule, "ThO2");

    // Print ThO2 molecule
    sem_post(&sem_print_molecule_type);

    // Change order of molecule composition
    sem_post(&sem_compose_co2);
  }
}

void *print_molecule_type(void *arg) {
  while (1) {
    // Wait for print_molecule_type semaphore to be posted
    sem_wait(&sem_print_molecule_type);

    // Check if composed molecule is empty
    if (strcmp(compose_molecule, "") == 0) {
      continue;
    }
    // Print composed molecule
    printf("Composed molecule: %s\n", compose_molecule);

    // Reset composed molecule
    strcpy(compose_molecule, "");

    sem_post(&sem_print_molecule_type);
  }
}
int main(int argc, char *argv[]) {
  int opt;

  // Seed random number generator
  unsigned int seed = 42;
  srand(seed);

  // Parse command line arguments
  while ((opt = getopt(argc, argv, "m:g:")) != -1) {
    switch (opt) {
        // Number of all atoms
      case 'm':
        number_m = atoi(optarg);
        break;
        // Rate of generation
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

  // Initialize atoms based on numbeer of m
  atoms_c = malloc(number_m / 6 * sizeof(struct atom));
  atoms_n = malloc(number_m / 6 * sizeof(struct atom));
  atoms_s = malloc(number_m / 6 * sizeof(struct atom));
  atoms_th = malloc(number_m / 6 * sizeof(struct atom));
  atoms_o = malloc(number_m / 3 * sizeof(struct atom));

  // Initialize semaphores for each atom
  sem_init(&sem_produce_c, 0, 0);
  sem_init(&sem_produce_n, 0, 0);
  sem_init(&sem_produce_s, 0, 0);
  sem_init(&sem_produce_th, 0, 0);
  sem_init(&sem_produce_o, 0, 0);

  // Initialize semaphores for each molecule
  sem_init(&sem_compose_co2, 0, 0);
  sem_init(&sem_compose_no2, 0, 0);
  sem_init(&sem_compose_so2, 0, 0);
  sem_init(&sem_compose_tho2, 0, 0);

  sem_init(&sem_print_molecule_type, 0, 0);

  // Post semaphores for CO2 to start
  sem_post(&sem_compose_co2);

  // Create threads
  pthread_t t_produce_c, t_produce_n, t_produce_s, t_produce_th, t_produce_o;
  pthread_t t_compose_co2, t_compose_no2, t_compose_so2, t_compose_tho2,
      t_print;

  // Create threads for each atom type
  pthread_create(&t_produce_c, NULL, Produce_C, NULL);
  pthread_create(&t_produce_n, NULL, Produce_N, NULL);
  pthread_create(&t_produce_s, NULL, Produce_S, NULL);
  pthread_create(&t_produce_th, NULL, Produce_TH, NULL);
  pthread_create(&t_produce_o, NULL, Produce_O, NULL);

  // Create threads for each molecule type
  pthread_create(&t_compose_co2, NULL, Composer_CO2, NULL);
  pthread_create(&t_compose_no2, NULL, Composer_NO2, NULL);
  pthread_create(&t_compose_so2, NULL, Composer_SO2, NULL);
  pthread_create(&t_compose_tho2, NULL, Composer_ThO2, NULL);
  pthread_create(&t_print, NULL, print_molecule_type, NULL);

  // Wait for threads for each atom type
  pthread_join(t_produce_c, NULL);
  pthread_join(t_produce_n, NULL);
  pthread_join(t_produce_s, NULL);
  pthread_join(t_produce_th, NULL);
  pthread_join(t_produce_o, NULL);

  // Cancel threads for each molecule type
  pthread_cancel(t_compose_co2);
  pthread_cancel(t_compose_no2);
  pthread_cancel(t_compose_so2);
  pthread_cancel(t_compose_tho2);

  // Print wasted atoms
  for (int i = 0; i < NUM_ATOM_TYPES; i++) {
    for (int j = 0; j < current_atoms_count[i]; j++) {
      printf("%c with ID: %d is wasted\n", atom_types[i], atoms_c[j].atomID);
    }
  }

  // Cancel print thread
  pthread_cancel(t_print);

  // Destroy semaphores for each atom
  sem_destroy(&sem_produce_c);
  sem_destroy(&sem_produce_n);
  sem_destroy(&sem_produce_s);
  sem_destroy(&sem_produce_th);
  sem_destroy(&sem_produce_o);

  // Destroy semaphores for each molecule
  sem_destroy(&sem_compose_co2);
  sem_destroy(&sem_compose_no2);
  sem_destroy(&sem_compose_so2);
  sem_destroy(&sem_compose_tho2);

  // Free memory
  free(atoms_c);
  free(atoms_n);
  free(atoms_s);
  free(atoms_th);
  free(atoms_o);

  return 0;
}
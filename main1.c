// Created by: Deniz Karakay
// Created on: 28 April 2023
// Last modified on: 30 April 2023

#include <getopt.h>
#include <math.h>
#include <pthread.h>
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
int count_c = 20, count_n = 20, count_s = 20, count_o = 20, count_th = 20;
int rate_g = 100;

char compose_molecule[5] = "";

// Atom types
char atom_types[NUM_ATOM_TYPES] = {'C', 'N', 'S', 'T', 'O'};
int current_atoms_count[NUM_ATOM_TYPES] = {0, 0, 0, 0, 0};

// Molecule order
char molecule_order[5] = {'C', 'N', 'C', 'S', 'T'};
int molecule_order_index = 0;

// Mutex and condition variables
pthread_mutex_t my_mutex;
pthread_cond_t compose_co2, compose_no2, compose_so2, compose_tho2,
    print_molecule;

// Atoms arrays for each type
struct atom *atoms_c, *atoms_n, *atoms_s, *atoms_th, *atoms_o;

// Compose molecule functions
void* compose_co2_molecule(void* arg) {
  while (1) {
    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    // Wait until there are enough atoms to compose CO2 and the appropriate
    // molecule order
    while (current_atoms_count[0] < 1 || current_atoms_count[4] < 2 ||
           molecule_order[molecule_order_index] != 'C') {
      pthread_cond_wait(&compose_co2, &my_mutex);
    }

    // Compose CO2 molecule
    current_atoms_count[0] -= 1;
    current_atoms_count[4] -= 2;

    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    strcpy(compose_molecule, "CO2");
    pthread_cond_signal(&print_molecule);

    // Signal other threads based on the molecule order
    if (molecule_order_index == 1) {
      pthread_cond_signal(&compose_no2);
    } else if (molecule_order_index == 3) {
      pthread_cond_signal(&compose_so2);
    }

    // Unlock mutex
    pthread_mutex_unlock(&my_mutex);
  }
}

void* compose_no2_molecule(void* arg) {
  while (1) {
    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    // Wait until there are enough atoms to compose NO2 and the appropriate
    // molecule order
    while (current_atoms_count[1] < 1 || current_atoms_count[4] < 2 ||
           molecule_order[molecule_order_index] != 'N') {
      pthread_cond_wait(&compose_no2, &my_mutex);
    }

    // Compose NO2 molecule
    current_atoms_count[1] -= 1;
    current_atoms_count[4] -= 2;

    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    strcpy(compose_molecule, "NO2");
    pthread_cond_signal(&print_molecule);

    // Signal other threads based on the molecule order
    pthread_cond_signal(&compose_co2);
    pthread_mutex_unlock(&my_mutex);
  }
}

void* compose_so2_molecule(void* arg) {
  while (1) {
    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    // Wait until there are enough atoms to compose SO2 and the appropriate
    // molecule order
    while (current_atoms_count[2] < 1 || current_atoms_count[4] < 2 ||
           molecule_order[molecule_order_index] != 'S') {
      pthread_cond_wait(&compose_so2, &my_mutex);
    }

    // Compose SO2 molecule
    current_atoms_count[2] -= 1;
    current_atoms_count[4] -= 2;

    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    strcpy(compose_molecule, "SO2");
    pthread_cond_signal(&print_molecule);

    // Signal other threads based on the molecule order
    pthread_cond_signal(&compose_tho2);
    pthread_mutex_unlock(&my_mutex);
  }
}

void* compose_tho2_molecule(void* arg) {
  while (1) {
    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    // Wait until there are enough atoms to compose THO2 and the appropriate
    // molecule order
    while (current_atoms_count[3] < 1 || current_atoms_count[4] < 2 ||
           molecule_order[molecule_order_index] != 'T') {
      pthread_cond_wait(&compose_tho2, &my_mutex);
    }

    // Compose THO2 molecule
    current_atoms_count[3] -= 1;
    current_atoms_count[4] -= 2;

    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    strcpy(compose_molecule, "THO2");
    pthread_cond_signal(&print_molecule);

    // Signal other threads based on the molecule order
    pthread_cond_signal(&compose_co2);
    pthread_mutex_unlock(&my_mutex);
  }
}

void* print_molecule_type(void* arg) {
  while (1) {
    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    // Wait until a molecule is composed
    while (strlen(compose_molecule) == 0) {
      pthread_cond_wait(&print_molecule, &my_mutex);
    }

    // Print composed molecule
    printf("Composed molecule: %s\n", compose_molecule);

    // Reset composed molecule
    strcpy(compose_molecule, "");

    pthread_mutex_unlock(&my_mutex);
  }
}

// Custom sleep function
void sleep_func() {
  double sleep_time = -log(1 - (double)rand() / RAND_MAX) / rate_g;
  usleep(sleep_time * 1000000);
}

int main(int argc, char* argv[]) {
  int opt;

  // Seed random number generator
  unsigned int seed = 42;
  srand(seed);

  // Parse command line arguments
  while ((opt = getopt(argc, argv, "c:n:s:t:o:g:")) != -1) {
    switch (opt) {
      case 'c':
        count_c = atoi(optarg);
        break;
      case 'n':
        count_n = atoi(optarg);
        break;
      case 's':
        count_s = atoi(optarg);
        break;
      case 't':
        count_th = atoi(optarg);
        break;
      case 'o':
        count_o = atoi(optarg);
        break;
      case 'g':
        rate_g = atoi(optarg);
        break;
      default:
        break;
    }
  }
  // Print counts and rate
  printf("C: %d, N: %d, S: %d, TH: %d, O: %d, rate_g: %d\n", count_c, count_n,
         count_s, count_th, count_o, rate_g);

  // Initialize threads
  pthread_t t_CO2, t_NO2, t_SO2, t_THO2, t_print_molecule_type;

  // Initialize mutex and condition variables
  pthread_mutex_init(&my_mutex, NULL);
  pthread_cond_init(&compose_co2, NULL);
  pthread_cond_init(&compose_no2, NULL);
  pthread_cond_init(&compose_so2, NULL);
  pthread_cond_init(&compose_tho2, NULL);

  // Create threads
  pthread_create(&t_CO2, NULL, compose_co2_molecule, NULL);
  pthread_create(&t_NO2, NULL, compose_no2_molecule, NULL);
  pthread_create(&t_SO2, NULL, compose_so2_molecule, NULL);
  pthread_create(&t_THO2, NULL, compose_tho2_molecule, NULL);
  pthread_create(&t_print_molecule_type, NULL, print_molecule_type, NULL);

  // Sum of all atoms
  int total_atoms = count_c + count_n + count_s + count_o + count_th;

  // Array of all atoms
  int atom_count_all[NUM_ATOM_TYPES] = {count_c, count_n, count_s, count_th,
                                        count_o};

  // Create dynamic arrays of atoms for each type of atom
  atoms_c = malloc(count_c * sizeof(struct atom));
  atoms_n = malloc(count_n * sizeof(struct atom));
  atoms_s = malloc(count_s * sizeof(struct atom));
  atoms_th = malloc(count_th * sizeof(struct atom));
  atoms_o = malloc(count_o * sizeof(struct atom));

  // Atom ID counter
  int atomID = 1;

  for (int i = 0; i < total_atoms; i++) {
    //  Randomly select an atom type
    int atom_type = rand() % NUM_ATOM_TYPES;
    char atom_type_char = atom_types[atom_type];

    // If there are no atoms of this type, randomly select another type
    while (atom_count_all[atom_type] == 0) {
      atom_type = rand() % NUM_ATOM_TYPES;
      atom_type_char = atom_types[atom_type];
    }

    // Create atom
    struct atom new_atom;
    new_atom.atomID = atomID++;
    new_atom.atomTYPE = atom_type_char;

    printf("%c with ID: %d is created\n", new_atom.atomTYPE, new_atom.atomID);

    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    switch (new_atom.atomTYPE) {
      case 'C':
        // Store atom in array
        atoms_c[current_atoms_count[0]] = new_atom;
        // Decrement atom count
        atom_count_all[0]--;

        // Increment current atom count
        current_atoms_count[0]++;

        // Signal thread
        pthread_cond_signal(&compose_co2);

        break;

      case 'N':
        // Store atom in array
        atoms_n[current_atoms_count[1]] = new_atom;

        // Decrement atom count
        atom_count_all[1]--;

        // Increment current atom count
        current_atoms_count[1]++;

        // Signal thread
        pthread_cond_signal(&compose_no2);

        break;

      case 'S':
        // Store atom in array
        atoms_s[current_atoms_count[2]] = new_atom;

        // Decrement atom count
        atom_count_all[2]--;

        // Increment current atom count
        current_atoms_count[2]++;

        // Signal thread
        pthread_cond_signal(&compose_so2);

        break;

      case 'T':
        // Store atom in array
        atoms_th[current_atoms_count[3]] = new_atom;

        // Decrement atom count
        atom_count_all[3]--;

        // Increment current atom count
        current_atoms_count[3]++;

        // Signal thread
        pthread_cond_signal(&compose_tho2);

        break;

      case 'O':
        // Store atom in array
        atoms_o[current_atoms_count[4]] = new_atom;

        // Decrement atom count
        atom_count_all[4]--;

        // Increment current atom count
        current_atoms_count[4]++;

        // Signal threads
        pthread_cond_signal(&compose_co2);
        pthread_cond_signal(&compose_no2);
        pthread_cond_signal(&compose_so2);
        pthread_cond_signal(&compose_tho2);

        break;

      default:
        break;
    }

    // Unlock mutex
    pthread_mutex_unlock(&my_mutex);

    // Sleep for a random amount of time
    sleep_func();
  }

  // Waste all remaining atoms
  int temp_c = count_c - current_atoms_count[0];
  for (int i = temp_c; i < current_atoms_count[0]; i++) {
    printf("C with ID: %d is wasted\n", atoms_c[i].atomID);
  }

  int temp_n = count_n - current_atoms_count[1];
  for (int i = temp_n; i < current_atoms_count[1]; i++) {
    printf("N with ID: %d is wasted\n", atoms_n[i].atomID);
  }

  int temp_s = count_s - current_atoms_count[2];
  for (int i = temp_s; i < current_atoms_count[2]; i++) {
    printf("S with ID: %d is wasted\n", atoms_s[i].atomID);
  }

  int temp_th = count_th - current_atoms_count[3];
  for (int i = temp_th; i < current_atoms_count[3]; i++) {
    printf("T with ID: %d is wasted\n", atoms_th[i].atomID);
  }

  int temp_o = count_o - current_atoms_count[4];
  for (int i = temp_o; i < current_atoms_count[4]; i++) {
    printf("O with ID: %d is wasted\n", atoms_o[i].atomID);
  }

  // Cancel threads
  pthread_cancel(t_CO2);
  pthread_cancel(t_NO2);
  pthread_cancel(t_SO2);
  pthread_cancel(t_THO2);

  // Cancel print molecule thread
  pthread_cancel(t_print_molecule_type);

  // Destroy mutex and condition variables
  pthread_mutex_destroy(&my_mutex);
  pthread_cond_destroy(&compose_co2);
  pthread_cond_destroy(&compose_no2);
  pthread_cond_destroy(&compose_so2);
  pthread_cond_destroy(&compose_tho2);

  // Free memory
  free(atoms_c);
  free(atoms_n);
  free(atoms_s);
  free(atoms_th);
  free(atoms_o);

  return 0;
}
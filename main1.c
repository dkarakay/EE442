// Created by: Deniz Karakay
// Created on: 28 April 2023
// Last modified on: 29 April 2023

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

  // Array to store the index of each atom type`
  int atom_c_index[count_c];
  int atom_n_index[count_n];
  int atom_s_index[count_s];
  int atom_th_index[count_th];
  int atom_o_index[count_o];

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
        // Store atom index
        atom_c_index[current_atoms_count[0]] = new_atom.atomID;

        // Decrement atom count
        atom_count_all[0]--;

        // Increment current atom count
        current_atoms_count[0]++;

        // Signal thread
        pthread_cond_signal(&compose_co2);
        break;

      case 'N':
        // Store atom index
        atom_n_index[current_atoms_count[1]] = new_atom.atomID;

        // Decrement atom count
        atom_count_all[1]--;

        // Increment current atom count
        current_atoms_count[1]++;

        // Signal thread
        pthread_cond_signal(&compose_no2);
        break;

      case 'S':
        // Store atom index
        atom_s_index[current_atoms_count[2]] = new_atom.atomID;

        // Decrement atom count
        atom_count_all[2]--;

        // Increment current atom count
        current_atoms_count[2]++;

        // Signal thread
        pthread_cond_signal(&compose_so2);

      case 'T':
        // Store atom index
        atom_th_index[current_atoms_count[3]] = new_atom.atomID;

        // Decrement atom count
        atom_count_all[3]--;

        // Increment current atom count
        current_atoms_count[3]++;

        // Signal thread
        pthread_cond_signal(&compose_tho2);

      case 'O':
        // Store atom index
        atom_o_index[current_atoms_count[4]] = new_atom.atomID;

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

    /*printf("C: %d, N: %d, S: %d, TH: %d, O: %d\n", current_atoms_count[0],
           current_atoms_count[1], current_atoms_count[2],
           current_atoms_count[3], current_atoms_count[4]);*/

    pthread_mutex_unlock(&my_mutex);

    // Sleep for a random amount of time
    sleep_func();
  }

  // Waste all remaining atoms
  int temp_c = current_atoms_count[0];
  for (int i = 0; i < temp_c; i++) {
    printf("C with ID: %d is wasted\n", atom_c_index[i]);
  }

  int temp_n = current_atoms_count[1];
  for (int i = 0; i < temp_n; i++) {
    printf("N with ID: %d is wasted\n", atom_n_index[i]);
  }

  int temp_s = current_atoms_count[2];
  for (int i = 0; i < temp_s; i++) {
    printf("S with ID: %d is wasted\n", atom_s_index[i]);
  }

  int temp_th = current_atoms_count[3];
  for (int i = 0; i < temp_th; i++) {
    printf("TH with ID: %d is wasted\n", atom_th_index[i]);
  }

  int temp_o = current_atoms_count[4];
  for (int i = 0; i < temp_o; i++) {
    printf("O with ID: %d is wasted\n", atom_o_index[i]);
  }

  return 0;
}
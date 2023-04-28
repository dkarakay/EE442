#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_ATOM_TYPES 5

// Struct to hold atom
struct atom {
  int atomID;
  char atomTYPE;  // C, N, S, O or TH
};

struct atoms {
  struct atom* atoms_list;
  int max_size;
  int current_size;
};

struct atoms all_atoms[NUM_ATOM_TYPES];

struct co2 {
  struct atoms c;
  struct atoms o;
};

struct no2 {
  struct atoms n;
  struct atoms o;
};

struct so2 {
  struct atoms s;
  struct atoms o;
};

struct tho2 {
  struct atoms th;
  struct atoms o;
};

// Default values
int count_c = 20, count_n = 20, count_s = 20, count_o = 20, count_th = 20;
int rate_g = 100;

// Atom types
char atom_types[NUM_ATOM_TYPES] = {'C', 'N', 'S', 'T', 'O'};
// int current_atoms_count[NUM_ATOM_TYPES] = {0, 0, 0, 0, 0};

// Molecule order
char molecule_order[5] = {'C', 'N', 'C', 'S', 'T'};
int molecule_order_index = 0;

int temp = 0;

// Mutex and condition variables
pthread_mutex_t my_mutex;
pthread_cond_t compose_co2, compose_no2, compose_so2, compose_tho2;

// Compose molecule functions
void* compose_co2_molecule(void* arg) {
  while (1) {
    struct co2* selected = (struct co2*)arg;
    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    int curr_c = selected->c.current_size;
    int curr_o = selected->o.current_size;

    printf("curr_c: %d, curr_o: %d, molecule_order[molecule_order_index]: %c\n",
           curr_c, curr_o, molecule_order[molecule_order_index]);

    // Wait until there are enough atoms to compose CO2 and the appropriate
    // molecule order
    while (curr_c < 1 || curr_o < 2 ||
           molecule_order[molecule_order_index] != 'C') {
      pthread_cond_wait(&compose_co2, &my_mutex);
    }

    // Compose CO2 molecule
    selected->c.current_size -= 1;
    selected->o.current_size -= 2;

    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    printf("CO2 molecule composed\n");

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
    struct no2* selected = (struct no2*)arg;

    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    int curr_n = selected->n.current_size;
    int curr_o = selected->o.current_size;

    // Wait until there are enough atoms to compose NO2 and the appropriate
    // molecule order

    printf("curr_n: %d, curr_o: %d, molecule_order[molecule_order_index]: %c\n",
           curr_n, curr_o, molecule_order[molecule_order_index]);
    while (curr_n < 1 || curr_o < 2 ||
           molecule_order[molecule_order_index] != 'N') {
      pthread_cond_wait(&compose_no2, &my_mutex);
    }

    // Compose NO2 molecule
    selected->n.current_size -= 1;
    selected->o.current_size -= 2;
    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    printf("NO2 molecule composed\n");

    // Signal other threads based on the molecule order
    pthread_cond_signal(&compose_co2);
    pthread_mutex_unlock(&my_mutex);
  }
}

void* compose_so2_molecule(void* arg) {
  while (1) {
    struct so2* selected = (struct so2*)arg;

    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    int curr_s = selected->s.current_size;
    int curr_o = selected->o.current_size;

    // Wait until there are enough atoms to compose SO2 and the appropriate
    // molecule order
    while (curr_s < 1 || curr_o < 2 ||
           molecule_order[molecule_order_index] != 'S') {
      pthread_cond_wait(&compose_so2, &my_mutex);
    }

    // Compose SO2 molecule
    selected->s.current_size -= 1;
    selected->o.current_size -= 2;
    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    printf("SO2 molecule composed\n");

    // Signal other threads based on the molecule order
    pthread_cond_signal(&compose_tho2);
    pthread_mutex_unlock(&my_mutex);
  }
}

void* compose_tho2_molecule(void* arg) {
  while (1) {
    struct tho2* selected = (struct tho2*)arg;

    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    int curr_th = selected->th.current_size;
    int curr_o = selected->o.current_size;

    // Wait until there are enough atoms to compose THO2 and the appropriate
    // molecule order
    while (curr_th < 1 || curr_o < 2 ||
           molecule_order[molecule_order_index] != 'T') {
      pthread_cond_wait(&compose_tho2, &my_mutex);
    }

    // Compose THO2 molecule
    selected->th.current_size -= 1;
    selected->o.current_size -= 2;
    // Update molecule order index
    molecule_order_index = (molecule_order_index + 1) % 5;
    printf("THO2 molecule composed\n");

    // Signal other threads based on the molecule order
    pthread_cond_signal(&compose_co2);
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
  pthread_t t_CO2, t_NO2, t_SO2, t_THO2;

  // Initialize mutex and condition variables
  pthread_mutex_init(&my_mutex, NULL);
  pthread_cond_init(&compose_co2, NULL);
  pthread_cond_init(&compose_no2, NULL);
  pthread_cond_init(&compose_so2, NULL);
  pthread_cond_init(&compose_tho2, NULL);

  // Sum of all atoms
  int total_atoms = count_c + count_n + count_s + count_o + count_th;

  // Array of all atoms
  int atom_counts[NUM_ATOM_TYPES] = {count_c, count_n, count_s, count_th,
                                     count_o};

  printf("Atom counts: %d, %d, %d, %d, %d\n", atom_counts[0], atom_counts[1],
         atom_counts[2], atom_counts[3], atom_counts[4]);

  printf("Total atoms: %d\n", total_atoms);

  for (int i = 0; i < NUM_ATOM_TYPES; i++) {
    all_atoms[i].atoms_list = malloc(sizeof(struct atom) * atom_counts[i]);
    all_atoms[i].max_size = atom_counts[i];
    all_atoms[i].current_size = 0;
  }

  struct co2 struct_co2;
  struct_co2.c = all_atoms[0];
  struct_co2.o = all_atoms[4];

  struct no2 struct_no2;
  struct_no2.o = all_atoms[4];
  struct_no2.n = all_atoms[1];

  struct so2 struct_so2;
  struct_so2.o = all_atoms[4];
  struct_so2.s = all_atoms[2];

  struct tho2 struct_tho2;
  struct_tho2.o = all_atoms[4];
  struct_tho2.th = all_atoms[3];

  /*pthread_create(&t_CO2, NULL, compose_co2_molecule, &struct_co2);
  pthread_create(&t_NO2, NULL, compose_no2_molecule, &struct_no2);
  pthread_create(&t_SO2, NULL, compose_so2_molecule, &struct_so2);
  pthread_create(&t_THO2, NULL, compose_tho2_molecule, &struct_tho2);*/

  // Atom ID counter
  int atomID = 1;

  for (int i = 0; i < total_atoms; i++) {
    //  Randomly select an atom type
    int atom_type = rand() % NUM_ATOM_TYPES;
    char atom_type_char = atom_types[atom_type];

    // If there are no atoms of this type, randomly select another type

    int max_size = all_atoms[atom_type].max_size;
    int current_size = all_atoms[atom_type].current_size;

    while (current_size == max_size) {
      atom_type = rand() % NUM_ATOM_TYPES;
      atom_type_char = atom_types[atom_type];
      max_size = all_atoms[atom_type].max_size;
      current_size = all_atoms[atom_type].current_size;
    }

    // Create atom
    struct atom new_atom;
    new_atom.atomID = atomID++;
    new_atom.atomTYPE = atom_type_char;

    // printf("%c with ID: %d is created\n", new_atom.atomTYPE,
    // new_atom.atomID);

    // Lock mutex
    pthread_mutex_lock(&my_mutex);

    switch (new_atom.atomTYPE) {
      case 'C':

        temp = all_atoms[0].current_size;
        all_atoms[0].atoms_list[temp] = new_atom;
        all_atoms[0].current_size++;

        // pthread_cond_signal(&compose_co2);
        break;

      case 'N':

        temp = all_atoms[1].current_size;
        all_atoms[1].atoms_list[temp] = new_atom;
        all_atoms[1].current_size++;

        // pthread_cond_signal(&compose_no2);
        break;

      case 'S':
        // atom_count_all[2]--;
        // current_atoms_count[2]++;
        temp = all_atoms[2].current_size;
        all_atoms[2].atoms_list[temp] = new_atom;
        all_atoms[2].current_size++;

        // pthread_cond_signal(&compose_so2);
        break;

      case 'T':
        // atom_count_all[3]--;
        // current_atoms_count[3]++;
        temp = all_atoms[3].current_size;
        all_atoms[3].atoms_list[temp] = new_atom;
        all_atoms[3].current_size++;

        // pthread_cond_signal(&compose_tho2);
        break;

      case 'O':
        // atom_count_all[4]--;
        // current_atoms_count[4]++;
        temp = all_atoms[4].current_size;
        all_atoms[4].atoms_list[temp] = new_atom;
        all_atoms[4].current_size++;

        pthread_cond_signal(&compose_co2);
        pthread_cond_signal(&compose_no2);
        pthread_cond_signal(&compose_so2);
        pthread_cond_signal(&compose_tho2);

      default:
        break;
    }

    printf("C: %d, N: %d, S: %d, TH: %d, O: %d\n", all_atoms[0].current_size,
           all_atoms[1].current_size, all_atoms[2].current_size,
           all_atoms[3].current_size, all_atoms[4].current_size);
    pthread_mutex_unlock(&my_mutex);

    // Sleep for a random amount of time
    sleep_func();

    if (all_atoms[0].current_size > 5) {
      pthread_create(&t_CO2, NULL, compose_co2_molecule, &struct_co2);
    }
  }

  return 0;
}

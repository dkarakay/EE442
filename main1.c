#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_NUM_ATOMS 1000
#define NUM_ATOM_TYPES 5

// Struct to hold atom
struct atom {
  int atomID;
  char atomTYPE;  // C, N, S, O or TH
};

struct atoms {
  struct atom* all_atoms;
  int max_size;
  int current_size;
  pthread_mutex_t the_mutex;
  pthread_cond_t generate_cond;
  pthread_cond_t waste_cond;
};

struct atoms atoms_c, atoms_n, atoms_s, atoms_o, atoms_th;

char atom_types[NUM_ATOM_TYPES] = {'C', 'N', 'S', 'O', 'T'};

// Default values
int count_c = 20, count_n = 20, count_s = 20, count_o = 20, count_th = 20;
int rate_g = 100;

int all_atoms_count = 0;

pthread_mutex_t molecule_mutex;
pthread_cond_t compose_co2;
pthread_cond_t compose_no2;
pthread_cond_t compose_so2;
pthread_cond_t compose_tho2;

void sleep_func() {
  double sleep_time = -log(1 - (double)rand() / RAND_MAX) / rate_g;
  usleep(sleep_time * 1000000);
}

void print_atoms(struct atoms* selected_atoms) {
  printf("Atom: %c, Curent size: %d, Max size: %d\n",
         selected_atoms->all_atoms[0].atomTYPE, selected_atoms->current_size,
         selected_atoms->max_size);
}

void* generate_atoms(void* arg) {
  struct atoms* selected_atoms = (struct atoms*)arg;
  char atom_type = selected_atoms->all_atoms[0].atomTYPE;
  int atoms_limit = selected_atoms->max_size;

  for (int i = 0; i < atoms_limit; i++) {
    pthread_mutex_lock(&selected_atoms->the_mutex);

    // Create new atom
    struct atom new_atom;
    new_atom.atomID = all_atoms_count + 1;
    all_atoms_count++;
    new_atom.atomTYPE = atom_type;
    selected_atoms->all_atoms[selected_atoms->current_size] = new_atom;
    selected_atoms->current_size = selected_atoms->current_size + 1;

    printf("Atom %d of type %c created by thread %lu\n", new_atom.atomID,
           new_atom.atomTYPE, pthread_self());

    /*if (selected_atoms->current_size >= 5) {
      pthread_cond_signal(&selected_atoms->waste_cond);
    }*/
    pthread_mutex_unlock(&selected_atoms->the_mutex);

    // print_atoms(selected_atoms);
    sleep_func();
  }
  printf("Thread %lu finished\n", pthread_self());
  return NULL;
}
/*
// Waste last atom only in thread
void* waste_last_atom(void* arg) {
  struct atoms* selected_atoms = (struct atoms*)arg;

  while (selected_atoms->current_size > 0) {
    pthread_mutex_lock(&selected_atoms->the_mutex);
    while (selected_atoms->current_size == 0) {
      pthread_cond_wait(&selected_atoms->waste_cond,
                        &selected_atoms->the_mutex);
    }
    printf(
        "Wasting last atom id %d of type %c\n",
        selected_atoms->all_atoms[selected_atoms->current_size - 1].atomID,
        selected_atoms->all_atoms[selected_atoms->current_size - 1].atomTYPE);
    selected_atoms->current_size--;
    selected_atoms->all_atoms[selected_atoms->current_size].atomID = -1;
    selected_atoms->all_atoms[selected_atoms->current_size].atomTYPE = 'X';
    pthread_cond_signal(&selected_atoms->generate_cond);
    pthread_mutex_unlock(&selected_atoms->the_mutex);
  }
}
*/

// Print all atoms id and type in one line
void print_all_atoms(struct atoms* selected_atoms) {
  printf("All %c atoms (%d): ", selected_atoms->all_atoms[0].atomTYPE,
         selected_atoms->current_size);
  for (int i = 0; i < selected_atoms->current_size; i++) {
    printf("%d ", selected_atoms->all_atoms[i].atomID);
  }
  printf("\n");
}

int molecule_order = 0;

void* compose_co2_molecule(void* arg) {
  while (1) {
    pthread_mutex_lock(&molecule_mutex);
    printf("Current size of C: %d, O: %d\n", atoms_c.current_size,
           atoms_o.current_size);
    while (atoms_c.current_size < 1 || atoms_o.current_size < 2) {
      printf("Waiting for C and O atoms\n");
      pthread_cond_wait(&compose_co2, &molecule_mutex);
    }

    if (molecule_order % 5 != 0) {
      printf("Molecule CO2 not composed by thread %lu\n", pthread_self());
      pthread_mutex_unlock(&molecule_mutex);
      continue;
    }

    atoms_c.current_size--;
    atoms_o.current_size -= 2;

    printf("Molecule CO2 composed by thread %lu\n", pthread_self());
    molecule_order++;
    pthread_cond_signal(&compose_no2);
    pthread_mutex_unlock(&molecule_mutex);
  }
}

void* compose_no2_molecule(void* arg) {
  while (1) {
    pthread_mutex_lock(&molecule_mutex);
    printf("Current size of N: %d, O: %d\n", atoms_n.current_size,
           atoms_o.current_size);
    while (atoms_n.current_size < 1 || atoms_o.current_size < 2) {
      printf("Waiting for N and O\n");
      pthread_cond_wait(&compose_no2, &molecule_mutex);
    }

    if (molecule_order % 5 != 1) {
      pthread_mutex_unlock(&molecule_mutex);
      continue;
    }

    atoms_n.current_size--;
    atoms_o.current_size -= 2;

    printf("Molecule NO2 composed by thread %lu\n", pthread_self());
    molecule_order--;
    pthread_cond_signal(&compose_co2);
    pthread_mutex_unlock(&molecule_mutex);
  }
}

int main(int argc, char* argv[]) {
  printf("Chemical Reaction Simulation\n");
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
  printf("C: %d, N: %d, S: %d, O: %d, TH: %d, G: %d\n", count_c, count_n,
         count_s, count_o, count_th, rate_g);

  atoms_c.all_atoms = malloc(count_c * sizeof(struct atom));
  atoms_c.max_size = count_c;
  atoms_c.current_size = 0;
  atoms_c.all_atoms[0].atomTYPE = atom_types[0];
  pthread_mutex_init(&atoms_c.the_mutex, NULL);
  pthread_cond_init(&atoms_c.waste_cond, NULL);

  atoms_n.all_atoms = malloc(count_n * sizeof(struct atom));
  atoms_n.max_size = count_n;
  atoms_n.current_size = 0;
  atoms_n.all_atoms[0].atomTYPE = atom_types[1];
  pthread_mutex_init(&atoms_n.the_mutex, NULL);
  pthread_cond_init(&atoms_n.waste_cond, NULL);

  /*atoms_s.all_atoms = malloc(count_s * sizeof(struct atom));
  atoms_s.max_size = count_s;
  atoms_s.current_size = 0;
  atoms_s.all_atoms[0].atomTYPE = atom_types[2];*/

  atoms_o.all_atoms = malloc(count_o * sizeof(struct atom));
  atoms_o.max_size = count_o;
  atoms_o.current_size = 0;
  atoms_o.all_atoms[0].atomTYPE = atom_types[3];
  pthread_mutex_init(&atoms_o.the_mutex, NULL);
  pthread_cond_init(&atoms_o.waste_cond, NULL);

  /*atoms_th.all_atoms = malloc(count_th * sizeof(struct atom));
  atoms_th.max_size = count_th;
  atoms_th.current_size = 0;
  atoms_th.all_atoms[0].atomTYPE = atom_types[4];*/

  pthread_t thread_c, thread_n, thread_s, thread_o, thread_th, thread_waste;
  pthread_t thread_co2, thread_no2;

  pthread_mutex_init(&molecule_mutex, NULL);
  pthread_cond_init(&compose_co2, NULL);

  pthread_create(&thread_c, NULL, generate_atoms, &atoms_c);
  pthread_create(&thread_n, NULL, generate_atoms, &atoms_n);
  // pthread_create(&thread_s, NULL, generate_atoms, &atoms_s);
  pthread_create(&thread_o, NULL, generate_atoms, &atoms_o);
  // pthread_create(&thread_th, NULL, generate_atoms, &atoms_th);

  pthread_create(&thread_co2, NULL, compose_co2_molecule, NULL);
  pthread_create(&thread_no2, NULL, compose_no2_molecule, NULL);
  // pthread_create(&thread_waste, NULL, waste_last_atom, &atoms_c);

  pthread_join(thread_c, NULL);
  pthread_join(thread_n, NULL);
  // pthread_join(thread_s, NULL);
  pthread_join(thread_o, NULL);
  // pthread_join(thread_th, NULL);
  //  pthread_join(thread_waste, NULL);

  pthread_join(thread_co2, NULL);
  pthread_join(thread_no2, NULL);

  pthread_mutex_destroy(&molecule_mutex);
  pthread_cond_destroy(&compose_co2);
  pthread_cond_destroy(&compose_no2);

  pthread_mutex_destroy(&atoms_c.the_mutex);
  pthread_cond_destroy(&atoms_c.waste_cond);
  pthread_mutex_destroy(&atoms_n.the_mutex);
  pthread_cond_destroy(&atoms_n.waste_cond);
  pthread_mutex_destroy(&atoms_o.the_mutex);
  pthread_cond_destroy(&atoms_o.waste_cond);

  print_all_atoms(&atoms_c);
  print_all_atoms(&atoms_n);
  print_all_atoms(&atoms_o);
  return 0;
}
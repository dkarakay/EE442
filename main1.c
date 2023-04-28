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
  int num_atoms;
  int current_size;
};

struct atoms atoms_c, atoms_n, atoms_s, atoms_o, atoms_th;

char atom_types[NUM_ATOM_TYPES] = {'C', 'N', 'S', 'O', 'T'};

// Default values
int count_c = 20, count_n = 20, count_s = 20, count_o = 20, count_th = 20,
    rate_g = 20;

int all_atoms_count = 0;

void* generate_atoms(void* arg) {
  struct atoms* selected_atoms = (struct atoms*)arg;
  char atom_type = selected_atoms->all_atoms[0].atomTYPE;
  int atoms_limit = selected_atoms->num_atoms;
  for (int i = 0; i < atoms_limit; i++) {
    struct atom new_atom;
    new_atom.atomID = all_atoms_count++;
    new_atom.atomTYPE = atom_type;
    selected_atoms->current_size++;
    printf("Atom %d of type %c created Current size: %d\n", new_atom.atomID,
           new_atom.atomTYPE, selected_atoms->current_size);
    double sleep_time = -log(1 - (double)rand() / RAND_MAX) / rate_g;
    usleep(sleep_time * 1000000);
  }

  /*char atom_type = *(char*)arg;
  int atoms_limit;
  switch (atom_type) {
    case 'C':
      atoms_limit = count_c;
      break;
    case 'N':
      atoms_limit = count_n;
      break;
    case 'S':
      atoms_limit = count_s;
      break;
    case 'O':
      atoms_limit = count_o;
      break;
    case 'T':
      atoms_limit = count_th;
      break;
    default:
      break;
  }

  for (int i = 0; i < atoms_limit; i++) {
    struct atom new_atom;
    new_atom.atomID = all_atoms_count++;
    new_atom.atomTYPE = atom_type;
    printf("Atom %d of type %c created\n", new_atom.atomID, new_atom.atomTYPE);
    double sleep_time = -log(1 - (double)rand() / RAND_MAX) / rate_g;
    usleep(sleep_time * 1000000);
  }*/
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
  atoms_c.num_atoms = count_c;
  atoms_c.current_size = 0;
  atoms_c.all_atoms[0].atomTYPE = 'C';

  atoms_n.all_atoms = malloc(count_n * sizeof(struct atom));
  atoms_n.num_atoms = count_n;
  atoms_n.current_size = 0;
  atoms_n.all_atoms[0].atomTYPE = 'N';

  pthread_t thread_c, thread_n, thread_s, thread_o, thread_th;

  pthread_create(&thread_c, NULL, generate_atoms, &atoms_c);
  pthread_create(&thread_n, NULL, generate_atoms, &atoms_n);

  pthread_join(thread_c, NULL);
  pthread_join(thread_n, NULL);
}
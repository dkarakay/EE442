/*
 * Created on Sun Jun 11 2023
 * Deniz Karakay - 2443307
 * EE442 - HW3
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_SIZE 512
#define FAT_SIZE 4096
#define FAT_ENTRY_SIZE 4
#define FILE_LIST_SIZE 128
#define FILE_NAME_SIZE 248

typedef struct {
  char data[DATA_SIZE];
} DataBlock;

typedef struct {
  DataBlock data_blocks[DATA_SIZE];
} Data;

typedef struct {
  unsigned int value;
} FatEntry;

typedef struct {
  FatEntry list_entries[FAT_SIZE];
} FAT;

typedef struct {
  char filename[FILE_NAME_SIZE];
  uint32_t start_cluster;
  uint32_t size;
} FileEntry;

typedef struct {
  FileEntry file_list[FILE_LIST_SIZE];
} FileList;

char *disk_location;
FILE *disk;

void Format() {
  disk = fopen(disk_location, "wb+");
  if (disk == NULL) {
    printf("Error: Could not open disk\n");
    return;
  }

  // Create FAT
  FAT fat;
  fat.list_entries[0].value = 0x0FFFFFFF;
  for (int i = 1; i < FAT_SIZE; i++) {
    fat.list_entries[i].value = 0x00000000;
  }

  // Create File List
  FileList file_list;
  for (int i = 0; i < FILE_LIST_SIZE; i++) {
    file_list.file_list[i].start_cluster = 0;
    file_list.file_list[i].size = 0;
    file_list.file_list[i].filename[0] = '\0';
  }

  // Write FAT and File List to disk
  fseek(disk, 0, SEEK_SET);
  fwrite(&fat, sizeof(FAT), 1, disk);
  fwrite(&file_list, sizeof(FileList), 1, disk);

  // Create Data Blocks
  Data data;
  for (int i = 0; i < DATA_SIZE; i++) {
    data.data_blocks[i].data[0] = '\0';
  }

  // Write Data Blocks to disk
  fseek(disk, sizeof(FAT) + sizeof(FileList), SEEK_SET);
  fwrite(&data, sizeof(Data), 1, disk);

  printf("Disk formatted\n");
}

void PrintFAT() {
  disk = fopen(disk_location, "rb+");

  FAT *t_fat = malloc(sizeof(FAT));
  fseek(disk, 0, SEEK_SET);
  fread(t_fat, sizeof(FAT), 1, disk);

  /*for (int i = 0; i < FAT_SIZE; i++) {
    printf("%d: %x\n", i, fat.list_entries[i].value);
  }*/

  FILE *fat_file = fopen("fat.txt", "w");
  for (int i = 0; i < FAT_SIZE; i++) {
    fprintf(fat_file, "%d: %x\n", i, t_fat->list_entries[i].value);
  }

  fclose(fat_file);
  fclose(disk);
  free(t_fat);
}

// Check if disk can be opened in a function
int OpenDisk(char *loc) {
  FILE *disk = fopen(loc, "rb+");
  if (disk == NULL) {
    printf("Error: Could not open disk\n");
    return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s folder_name -function [args]\n", argv[0]);
    return 1;
  }

  disk_location = argv[1];

  int disk_opened = OpenDisk(disk_location);
  printf("Loc: %s - Status: %d\n", disk_location, disk_opened);

  char *command = argv[2];
  printf("Command: %s\n", command);

  if (disk_opened == 0) {
    printf("Error: Could not open disk\n");
    return 1;
  }

  if (strcmp(command, "-format") == 0) {
    Format();
  } else if (strcmp(command, "-printfat") == 0) {
    PrintFAT();
  }

  return 0;
}
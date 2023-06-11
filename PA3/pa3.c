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
    uint32_t first_block;
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
        file_list.file_list[i].first_block = 0;
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

int FindFreeFatEntry(FAT *fat, int start_index) {
    for (int i = start_index; i < FAT_SIZE; i++) {
        if (fat->list_entries[i].value == 0x00000000) {
            return i;
        }
    }
    return -1;
}

int FindFreeFileListEntry(FileList *file_list) {
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        if (file_list->file_list[i].filename[0] == '\0') {
            return i;
        }
    }
    return -1;
}

int FindFileInList(FileList *file_list, char *file_name) {
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        if (strcmp(file_list->file_list[i].filename, file_name) == 0) {
            return i;
        }
    }
    return -1;
}

void Write(char *src_path, char *dest_file_name) {
    disk = fopen(disk_location, "rb+");

    // Read from Disk
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);

    Data *t_data = malloc(sizeof(Data));
    fseek(disk, sizeof(FAT) + sizeof(FileList), SEEK_SET);
    fread(t_data, sizeof(Data), 1, disk);

    FILE *src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
        printf("Error: Could not open source file\n");
        return;
    }

    // Find free FAT entries for the clusters
    int file_size = 0;
    int num_clusters = 0;
    while (!feof(src_file)) {
        DataBlock block;
        int bytes_read = fread(block.data, 1, DATA_SIZE, src_file);
        if (bytes_read > 0) {
            num_clusters++;
            file_size += bytes_read;
        }
    }

    printf("File size: %.2f MB\n", (float)file_size / (1024 * 1024));

    // Leave first FAT entry as it is
    int free_fat_index = 1;
    int last_fat_index = 0, first_fat_index = 0;
    for (int i = 0; i < num_clusters; i++) {
        free_fat_index = FindFreeFatEntry(t_fat, free_fat_index);
        if (free_fat_index == -1) {
            printf("Error: No free FAT entry\n");
            return;
        } else if (first_fat_index == 0) {
            first_fat_index = free_fat_index;
            printf("First FAT entry index: %d\n", first_fat_index);
        }
        t_fat->list_entries[last_fat_index].value = free_fat_index;
        last_fat_index = free_fat_index;
    }
    t_fat->list_entries[last_fat_index].value = 0xFFFFFFFF;


    printf("Last FAT entry index: %d\n", last_fat_index);
    printf("Number of clusters: %d\n", num_clusters);

    // Find free File List entry
    int free_file_list_index = FindFreeFileListEntry(t_file_list);
    if (free_file_list_index == -1) {
        printf("Error: No free File List entry\n");
        return;
    }

    // Update the File List with the file information
    FileEntry file_entry;
    strncpy(file_entry.filename, dest_file_name, FILE_NAME_SIZE);
    file_entry.first_block = first_fat_index;
    printf("First block: %d\n", file_entry.first_block);
    file_entry.size = file_size;
    t_file_list->file_list[free_file_list_index] = file_entry;

    // Write the file data to the disk
    fseek(disk,
          sizeof(FAT) + sizeof(FileList) +
              (file_entry.first_block - 1) * sizeof(Data),
          SEEK_SET);
    int bytes_written = 0;
    fseek(src_file, 0, SEEK_SET);
    while (!feof(src_file)) {
        DataBlock block;
        int bytes_read = fread(block.data, 1, DATA_SIZE, src_file);
        if (bytes_read > 0) {
            fwrite(block.data, 1, bytes_read, disk);
            bytes_written += bytes_read;
        }
    }

    // Write the updated FAT and File List to the disk
    fseek(disk, 0, SEEK_SET);
    fwrite(t_fat, sizeof(FAT), 1, disk);
    fwrite(t_file_list, sizeof(FileList), 1, disk);

    printf("%d bytes written to disk\n", bytes_written);

    fclose(src_file);
    fclose(disk);
    free(t_fat);
    free(t_file_list);
    free(t_data);
}
void Read(char *src_file_name, char *dest_path) {
    disk = fopen(disk_location, "rb");

    // Read from Disk
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);

    // Find the file in the File List
    int file_index = FindFileInList(t_file_list, src_file_name);
    if (file_index == -1) {
        printf("Error: File not found\n");
        return;
    }

    // Open the destination file for writing
    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
        printf("Error: Could not open destination file\n");
        return;
    }

    // Read the file data from the disk
    int current_block_index = 0;
    printf("First block index: %d\n", current_block_index);
    while (current_block_index != 0xFFFFFFFF) {
        printf("First block index: %d\n", current_block_index);
        fseek(disk,
              sizeof(FAT) + sizeof(FileList) +
                  (current_block_index - 1) * sizeof(Data),
              SEEK_SET);
        DataBlock block;
        fread(block.data, 1, DATA_SIZE, disk);
        fwrite(block.data, 1, DATA_SIZE, dest_file);
        current_block_index = t_fat->list_entries[current_block_index].value;
    }

    printf("File read successfully\n");

    fclose(dest_file);
    fclose(disk);
    free(t_fat);
    free(t_file_list);
}
void PrintFAT() {
    disk = fopen(disk_location, "rb+");

    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    FILE *fat_file = fopen("fat.txt", "w");
    for (int i = 0; i < FAT_SIZE; i++) {
        fprintf(fat_file, "%d: %x\n", i, t_fat->list_entries[i].value);
    }

    fclose(fat_file);
    fclose(disk);
    free(t_fat);
}

void PrintFileList() {
    disk = fopen(disk_location, "rb");

    // Read from Disk FAT
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    // Read from Disk File List
    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);

    // Write to file
    FILE *file_list_file = fopen("filelist.txt", "w");
    fprintf(file_list_file,
            "Item\tFilename\tFirst Block\t\tFile Size(bytes)\n");
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        fprintf(file_list_file, "%d\t\t%s\t\t%d\t\t\t%d\n", i,
                t_file_list->file_list[i].filename,
                t_file_list->file_list[i].first_block,
                t_file_list->file_list[i].size);
    }
    fclose(file_list_file);

    fclose(disk);
    free(t_fat);
    free(t_file_list);
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
    } else if (strcmp(command, "-write") == 0 && argc == 5) {
        Write(argv[3], argv[4]);
    } else if (strcmp(command, "-read") == 0 && argc == 5) {
        Read(argv[3], argv[4]);
    } else if (strcmp(command, "-printfat") == 0) {
        PrintFAT();
    }
    PrintFAT();
    PrintFileList();

    return 0;
}
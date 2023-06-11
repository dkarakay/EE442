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
    fat.list_entries[0].value = 0xFFFFFFFF;
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
        char *f_name = file_list->file_list[i].filename;
        if (strcmp(f_name, file_name) == 0) {
            return i;
        }
    }
    return -1;
}

uint32_t Big2LittleEndian(uint32_t value) {
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) |
           ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

uint32_t Little2BigEndian(uint32_t value) {
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) |
           ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
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

    FILE *src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
        printf("Error: Could not open source file\n");
        return;
    }

    // Find free file list entry
    int file_list_index = FindFreeFileListEntry(t_file_list);
    if (file_list_index == -1) {
        printf("Error: File list is full\n");
        return;
    }

    // Find free FAT entry
    int fat_index = FindFreeFatEntry(t_fat, 1);
    if (fat_index == -1) {
        printf("Error: Disk is full\n");
        return;
    }

    printf("FAT index: %d\n", fat_index);

    // Count free FAT entries
    int free_fat_entries = 0;
    for (int i = 0; i < FAT_SIZE; i++) {
        if (t_fat->list_entries[i].value == 0x00000000) {
            free_fat_entries++;
        }
    }

    printf("Free FAT entries: %d\n", free_fat_entries);

    int f_size = 0;
    int n_clusters = 0;
    while (!feof(src_file)) {
        DataBlock block;
        int b_read = fread(block.data, 1, DATA_SIZE, src_file);
        if (b_read > 0) {
            n_clusters++;
            f_size += b_read;
        }
    }

    src_file = fopen(src_path, "rb");

    printf("File size: %d\n", f_size);
    printf("Num clusters: %d\n", n_clusters);

    if (free_fat_entries < n_clusters) {
        printf("Error: Not enough free space on disk\n");
        return;
    }

    int num_clusters = 0;

    // Write file to disk
    FileEntry file_entry;
    file_entry.first_block = fat_index;
    file_entry.size = 0;
    strncpy(file_entry.filename, dest_file_name, FILE_NAME_SIZE);

    int data_index = fat_index;
    while (!feof(src_file)) {
        // Write data block to disk
        DataBlock data_block;
        int bytes_read = fread(data_block.data, 1, DATA_SIZE, src_file);
        if (bytes_read == 0) {
            break;
        }
        fseek(disk,
              sizeof(FAT) + sizeof(FileList) + (data_index * sizeof(DataBlock)),
              SEEK_SET);
        fwrite(&data_block, sizeof(DataBlock), 1, disk);

        // Update FAT
        uint32_t fat_value;
        if (bytes_read < DATA_SIZE) {
            fat_value = 0xFFFFFFFF;
        } else {
            fat_value = data_index + 1;
        }

        if (t_fat->list_entries[data_index].value == 0x00000000) {
            t_fat->list_entries[data_index].value = Big2LittleEndian(fat_value);
        }

        if (bytes_read > 0) {
            num_clusters++;
        }

        // Update file entry
        file_entry.size += bytes_read;

        // Move to next data block
        data_index = t_fat->list_entries[data_index].value;
        data_index = Little2BigEndian(data_index);
    }

    printf("File size: bytes: %d, clusters: %d\n", file_entry.size,
           num_clusters);

    // Update file list
    t_file_list->file_list[file_list_index] = file_entry;

    // Write FAT and File List to disk
    fseek(disk, 0, SEEK_SET);
    fwrite(t_fat, sizeof(FAT), 1, disk);
    fwrite(t_file_list, sizeof(FileList), 1, disk);

    fclose(src_file);
    printf("File written to disk\n");

    fclose(disk);
    free(t_fat);
    free(t_file_list);
}
void Read(char *src_file_name, char *dest_path) {
    disk = fopen(disk_location, "rb");

    // Read from Disk FAT
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    // Read from Disk File List
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

    // Read file from disk
    int data_index = t_file_list->file_list[file_index].first_block;
    int remaining_size = t_file_list->file_list[file_index].size;
    while (data_index != 0xFFFFFFFF && remaining_size > 0) {
        // Read data block from disk
        DataBlock data_block;
        fseek(disk,
              sizeof(FAT) + sizeof(FileList) + (data_index * sizeof(DataBlock)),
              SEEK_SET);
        fread(&data_block, sizeof(DataBlock), 1, disk);

        // Write data block to destination file
        int bytes_to_write =
            (remaining_size < DATA_SIZE) ? remaining_size : DATA_SIZE;
        fwrite(data_block.data, 1, bytes_to_write, dest_file);

        // Move to next data block
        data_index = t_fat->list_entries[data_index].value;
        remaining_size -= bytes_to_write;
    }

    fclose(dest_file);
    printf("File read from disk\n");
}

void List() {
    disk = fopen(disk_location, "rb");

    // Read from Disk File List
    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);

    printf("Filename\tSize\n");
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        char *filename = t_file_list->file_list[i].filename;
        int size = t_file_list->file_list[i].size;

        if (filename[0] != '.' && filename[0] != '\0') {
            printf("%-15s\t%d\n", filename, size);
        }
    }

    fclose(disk);
    free(t_file_list);
}

// Deletes a file from the disk
void Delete(char *src_file_name) {
    disk = fopen(disk_location, "rb+");

    // Read from Disk FAT
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    // Read from Disk File List
    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);

    printf("Filename to delete: %s\n", src_file_name);
    // Find the file in the File List
    int file_index = FindFileInList(t_file_list, src_file_name);
    printf("File index: %d\n", file_index);
    if (file_index == -1) {
        printf("Error: File not found\n");
        return;
    }

    // Free the FAT entries occupied by the file
    int block_index = t_file_list->file_list[file_index].first_block;
    printf("First block index: %d\n", block_index);
    while (block_index != 0xFFFFFFFF) {
        int next_block_index = t_fat->list_entries[block_index].value;
        next_block_index = Little2BigEndian(next_block_index);
        t_fat->list_entries[block_index].value = 0x00000000;
        block_index = next_block_index;
    }

    // Remove the file from the file list
    t_file_list->file_list[file_index].filename[0] = '\0';
    t_file_list->file_list[file_index].first_block = 0;
    t_file_list->file_list[file_index].size = 0;

    printf("File deleted: %s\n", src_file_name);

    // Write to Disk FAT
    fseek(disk, 0, SEEK_SET);
    fwrite(t_fat, sizeof(FAT), 1, disk);

    // Write to Disk File List
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(t_file_list, sizeof(FileList), 1, disk);

    fclose(disk);
    free(t_fat);
    free(t_file_list);
}

void SortA() {
    disk = fopen(disk_location, "rb+");

    // Read from Disk File List
    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);
    fclose(disk);

    // Sort file list by size in ascending order
    for (int i = 0; i < FILE_LIST_SIZE - 1; i++) {
        for (int j = i + 1; j < FILE_LIST_SIZE; j++) {
            int i_size = t_file_list->file_list[i].size;
            int j_size = t_file_list->file_list[j].size;

            if (i_size > j_size) {
                FileEntry temp = t_file_list->file_list[i];
                t_file_list->file_list[i] = t_file_list->file_list[j];
                t_file_list->file_list[j] = temp;
            }
        }
    }

    printf("Filename\tSize\n");
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        char *filename = t_file_list->file_list[i].filename;
        int size = t_file_list->file_list[i].size;

        if (filename[0] != '.' && filename[0] != '\0') {
            printf("%-15s\t%d\n", filename, size);
        }
    }

    free(t_file_list);
}

void SortD() {
    disk = fopen(disk_location, "rb+");

    // Read from Disk File List
    FileList *t_file_list = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_file_list, sizeof(FileList), 1, disk);
    fclose(disk);

    // Sort file list by size in ascending order
    for (int i = 0; i < FILE_LIST_SIZE - 1; i++) {
        for (int j = i + 1; j < FILE_LIST_SIZE; j++) {
            int i_size = t_file_list->file_list[i].size;
            int j_size = t_file_list->file_list[j].size;

            if (i_size < j_size) {
                FileEntry temp = t_file_list->file_list[i];
                t_file_list->file_list[i] = t_file_list->file_list[j];
                t_file_list->file_list[j] = temp;
            }
        }
    }

    printf("Filename\tSize\n");
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        char *filename = t_file_list->file_list[i].filename;
        int size = t_file_list->file_list[i].size;

        if (filename[0] != '.' && filename[0] != '\0') {
            printf("%-15s\t%d\n", filename, size);
        }
    }
    free(t_file_list);
}

void PrintFAT() {
    disk = fopen(disk_location, "rb+");

    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    FILE *fat_file = fopen("fat.txt", "w");
    fprintf(fat_file,
            "Entry\tValue\t\tEntry\tValue\t\tEntry\tValue\t\tEntry\tValue\n");
    for (int i = 0; i < FAT_SIZE; i += 4) {
        fprintf(fat_file, "%04x\t%08x\t%04x\t%08x\t%04x\t%08x\t%04x\t%08x\n", i,
                t_fat->list_entries[i].value, i + 1,
                t_fat->list_entries[i + 1].value, i + 2,
                t_fat->list_entries[i + 2].value, i + 3,
                t_fat->list_entries[i + 3].value);
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
            "Item\tFilename\t\tFirst Block\t\tFile Size(bytes)\n");
    for (int i = 0; i < FILE_LIST_SIZE; i++) {
        char *filename = t_file_list->file_list[i].filename;
        int first_block = t_file_list->file_list[i].first_block;
        int size = t_file_list->file_list[i].size;

        if (filename[0] == '\0') {
            filename = "NULL";
        }

        fprintf(file_list_file, "%d\t\t%-15s\t%4d\t\t\t%d\n", i, filename,
                first_block, size);
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
    char *command = argv[2];

    if (disk_opened == 0) {
        printf("Error: Could not open disk\n");
        return 1;
    }

    if (strcmp(command, "-format") == 0) {
        Format();
    } else if (strcmp(command, "-list") == 0 && argc == 3) {
        List();
    } else if (strcmp(command, "-write") == 0 && argc == 5) {
        Write(argv[3], argv[4]);
    } else if (strcmp(command, "-read") == 0 && argc == 5) {
        Read(argv[3], argv[4]);
    } else if (strcmp(command, "-printfat") == 0) {
        PrintFAT();
    } else if (strcmp(command, "-printfilelist") == 0) {
        PrintFileList();
    } else if (strcmp(command, "-delete") == 0 && argc == 4) {
        Delete(argv[3]);
    } else if (strcmp(command, "-sorta") == 0) {
        SortA();
    } else if (strcmp(command, "-sortd") == 0) {
        SortD();
    } else {
        printf("Error: Invalid command\n");
        return 1;
    }
    PrintFAT();
    PrintFileList();

    return 0;
}
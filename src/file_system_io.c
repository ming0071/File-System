#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.h"

// 內部輔助函數
static void saveInodeRecursive(FILE *file, Inode *inode) {
    // Save the basic properties of the inode
    size_t len = strlen(inode->name) + 1;
    fwrite(&len, sizeof(size_t), 1, file);          // Save name length
    fwrite(inode->name, sizeof(char), len, file);   // Save name
    fwrite(&inode->is_directory, sizeof(int), 1, file);
    fwrite(&inode->file_size, sizeof(size_t), 1, file);
    fwrite(&inode->start_block, sizeof(int), 1, file);
    fwrite(&inode->block_count, sizeof(size_t), 1, file);
    fwrite(&inode->directory_item_count, sizeof(size_t), 1, file);

    // Save directory items if it is a directory
    if (inode->is_directory && inode->directory_items) {
        for (size_t i = 0; i < inode->directory_item_count; i++)
            saveInodeRecursive(file, inode->directory_items[i]);
    }
}

void saveFileSystem(FileSystem *fs, const char *password) {
    FILE *file = fopen("data/filesystem.dump", "wb"); // 改為 data 目錄
    if (!file) { perror("Failed to open dump file"); return; }

    // Save the 6-digit password
    fwrite(password, sizeof(char), 6, file);

    // Save the FileSystem metadata
    fwrite(&fs->partition_size, sizeof(size_t), 1, file);
    fwrite(&fs->block_count, sizeof(size_t), 1, file);
    fwrite(&fs->block_used, sizeof(size_t), 1, file);
    fwrite(fs->block_bitmap, sizeof(int), fs->block_count, file);
    fwrite(&fs->inode_count, sizeof(size_t), 1, file);
    fwrite(&fs->inode_used, sizeof(size_t), 1, file);

    // Save the inode tree starting from the root
    saveInodeRecursive(file, fs->root);

    // Save the data blocks
    fwrite(fs->data_blocks, BLOCK_SIZE, fs->block_count, file);

    fclose(file);
    printf("File system has been saved to 'data/filesystem.dump'with password.\n");
}

static void loadInodeRecursive(FILE *file, Inode **inode, Inode *parent) {
    // Allocate memory for the inode structure
    *inode = (Inode *)malloc(sizeof(Inode));
    if (!*inode) // Check if memory allocation was successful
    {
        printf("Failed to allocate memory for inode.\n");
        return;
    }

    // Read and set the inode's name length from the file
    size_t name_length;
    fread(&name_length, sizeof(size_t), 1, file); // Read the length of the name
    // Allocate memory for the inode's name based on the read length
    (*inode)->name = (char *)malloc(name_length);
    // Read the actual name from the file
    fread((*inode)->name, sizeof(char), name_length, file);

    // Read basic properties of the inode from the file
    fread(&(*inode)->is_directory, sizeof(int), 1, file);            // Read if it is a directory
    fread(&(*inode)->file_size, sizeof(size_t), 1, file);            // Read the file size
    fread(&(*inode)->start_block, sizeof(int), 1, file);             // Read the start block index
    fread(&(*inode)->block_count, sizeof(size_t), 1, file);          // Read the number of blocks
    fread(&(*inode)->directory_item_count, sizeof(size_t), 1, file); // Read the number of directory items

    // Set the parent inode to establish the directory hierarchy
    (*inode)->parent = parent;

    // If the inode represents a directory, recursively load its items
    if ((*inode)->is_directory)
    {
        // Allocate memory for an array of directory items (child inodes)
        (*inode)->directory_items = (Inode **)malloc((*inode)->directory_item_count * sizeof(Inode *));
        for (size_t i = 0; i < (*inode)->directory_item_count; i++)
        {
            // Recursively load each child inode
            loadInodeRecursive(file, &(*inode)->directory_items[i], *inode);
        }
    }
    else
    {
        // If it is not a directory, set directory_items to NULL
        (*inode)->directory_items = NULL;
    }
}

void loadFileSystem(FileSystem **fs, const char *inputPassword) {
    FILE *file = fopen("data/filesystem.dump", "rb");
    if (!file) { printf("Dump not found.\n"); return; }

    // Read and verify the password
    char storedPwd[7] = {0};    // Extra space for null terminator
    fread(storedPwd, sizeof(char), 6, file);
    if (strncmp(storedPwd, inputPassword, 6) != 0) {
        printf("Wrong password.\n"); fclose(file); return;
    }

    // Allocate memory for the FileSystem
    *fs = (FileSystem *)malloc(sizeof(FileSystem));
    if (!*fs)
    {
        printf("Failed to allocate memory for file system.\n");
        fclose(file);
        return;
    }

    // Load the FileSystem metadata
    fread(&(*fs)->partition_size, sizeof(size_t), 1, file);
    fread(&(*fs)->block_count, sizeof(size_t), 1, file);
    fread(&(*fs)->block_used, sizeof(size_t), 1, file);
    (*fs)->block_bitmap = (int *)malloc((*fs)->block_count * sizeof(int));
    fread((*fs)->block_bitmap, sizeof(int), (*fs)->block_count, file);
    fread(&(*fs)->inode_count, sizeof(size_t), 1, file);
    fread(&(*fs)->inode_used, sizeof(size_t), 1, file);

    // Initialize the inodes array
    (*fs)->inodes = (Inode **)calloc((*fs)->inode_count, sizeof(Inode *));
    
    // Load the inode tree starting from the root
    loadInodeRecursive(file, &(*fs)->root, NULL);
    (*fs)->inodes[0] = (*fs)->root;
    (*fs)->current_directory = (*fs)->root;

    // Load the data blocks
    (*fs)->data_blocks = (char *)malloc((*fs)->block_count * BLOCK_SIZE);
    fread((*fs)->data_blocks, BLOCK_SIZE, (*fs)->block_count, file);
    fclose(file);
    printf("File system has been loaded from 'data/filesystem.dump'.\n");
}
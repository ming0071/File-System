#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "file_system.h"

FileSystem *createFileSystem(size_t size)
{
    FileSystem *fs = (FileSystem *)malloc(sizeof(FileSystem));
    if (!fs)
        return NULL;
    fs->partition_size = size;
    fs->block_count = size / BLOCK_SIZE;
    fs->block_used = 0;
    fs->data_blocks = (char *)malloc(fs->block_count * BLOCK_SIZE);
    fs->block_bitmap = (int *)calloc(fs->block_count, sizeof(int));
    fs->inode_count = size / INODE_PER_PARTITION;
    fs->inode_used = 1;
    fs->inodes = (Inode **)calloc(fs->inode_count, sizeof(Inode *));

    // initialize root directory
    Inode *root = (Inode *)malloc(sizeof(Inode));
    root->name = strdup("/");
    root->is_directory = 1;
    root->file_size = 0;
    root->start_block = -1;
    root->block_count = 0;
    root->parent = NULL;
    root->directory_items = NULL;
    root->directory_item_count = 0;

    fs->root = root;
    fs->inodes[0] = root;
    fs->current_directory = root;
    return fs;
}

void printCurrentPath(Inode *current)
{
    if (current->parent == NULL)
    {
        printf("/");
        return;
    }
    printCurrentPath(current->parent);
    printf("%s/", current->name);
}

void ls(FileSystem *fs)
{
    if (!fs->current_directory->is_directory)
    {
        printf("Current directory is not a directory.\n");
        return;
    }
    else if (fs->current_directory->directory_item_count == 0)
    {
        printf("No files or directories found.\n");
        return;
    }

    size_t count = 0;
    for (size_t i = 0; i < fs->current_directory->directory_item_count; ++i)
    {
        Inode *inode = fs->current_directory->directory_items[i];
        printf("%s%-*s%s ", inode->is_directory ? COLOR_BLUE : COLOR_WHITE, COLUMN_WIDTH, inode->name, COLOR_RESET);
        count++;
        if (count % 6 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void cd(FileSystem *fs, const char *path)
{
    if (strcmp(path, "..") == 0)
    {
        if (fs->current_directory->parent)
            fs->current_directory = fs->current_directory->parent;
        return;
    }
    // search for the target directory
    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        if (strcmp(fs->current_directory->directory_items[i]->name, path) == 0 && fs->current_directory->directory_items[i]->is_directory)
        {
            fs->current_directory = fs->current_directory->directory_items[i];
            return;
        }
    }
    printf("Directory not found.\n");
}

void my_mkdir(FileSystem *fs, const char *dirname)
{
    for (size_t i = 0; i < fs->current_directory->directory_item_count; ++i)
    {
        Inode *inode = fs->current_directory->directory_items[i];
        if (inode->is_directory && strcmp(inode->name, dirname) == 0)
        {
            printf("Directory '%s' already exists.\n", dirname);
            return;
        }
    }

    // init new directory inode
    Inode *new_dir = (Inode *)malloc(sizeof(Inode));
    new_dir->name = strdup(dirname);
    new_dir->is_directory = 1;
    new_dir->file_size = 0;
    new_dir->start_block = -1;
    new_dir->block_count = 0;
    new_dir->parent = fs->current_directory;
    new_dir->directory_items = NULL;
    new_dir->directory_item_count = 0;
    
    fs->inodes[fs->inode_used] = new_dir;
    fs->inode_used++;
    fs->block_used++;

    // Add the new directory to the current directory's directory_items array
    if (fs->current_directory->is_directory)
    {
        fs->current_directory->directory_item_count++;
        fs->current_directory->directory_items = realloc(fs->current_directory->directory_items, fs->current_directory->directory_item_count * sizeof(Inode *));
        fs->current_directory->directory_items[fs->current_directory->directory_item_count - 1] = new_dir;
    }
}

void delete_inode(FileSystem *fs, Inode *inode) {
    if (!inode) return;

    // Recursively delete all child inodes, if it's a directory
    if (inode->is_directory && inode->directory_items) {
        for (size_t i = 0; i < inode->directory_item_count; i++) {
            delete_inode(fs, inode->directory_items[i]);
        }
        free(inode->directory_items);
        inode->directory_items = NULL;
        inode->directory_item_count = 0;
    }
    else if (!inode->is_directory) {
        // if it's a file, clear the data blocks in the block bitmap
        if (inode->start_block != -1) {
            for (size_t j = 0; j < inode->block_count; j++) {
                int block_index = inode->start_block + j;
                if (block_index < (int)fs->block_count) {
                    fs->block_bitmap[block_index] = 0;
                }
            }
            fs->block_used -= inode->block_count;
        }
    }

    // Remove the inode from the global inodes array
    for (size_t k = 0; k < fs->inode_count; k++) {
        if (fs->inodes[k] == inode) {
            fs->inodes[k] = NULL;
            fs->inode_used--;
            break;
        }
    }

    free(inode->name);
    free(inode);
}

void my_rmdir(FileSystem *fs, const char *dirname)
{
    Inode *target = NULL;
    size_t target_index = 0;

    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        Inode *item = fs->current_directory->directory_items[i];
        if (strcmp(item->name, dirname) == 0 && item->is_directory)
        {
            target = item;
            target_index = i;
            break;
        }
    }
    if (!target)
    {
        printf("Directory '%s' not found.\n", dirname);
        return;
    }

    delete_inode(fs, target);

    // Remove the target directory from the current directory's directory_items array
    for (size_t i = target_index; i < fs->current_directory->directory_item_count - 1; i++)
    {
        fs->current_directory->directory_items[i] = fs->current_directory->directory_items[i + 1];
    }
    fs->current_directory->directory_item_count--;

    printf("Directory '%s' and its contents have been removed.\n", dirname);
}

void touch(FileSystem *fs, const char *fileName)
{
    // Check if a file with the same name already exists
    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        if (strcmp(fs->current_directory->directory_items[i]->name, fileName) == 0)
        {
            printf("File or directory with name '%s' already exists.\n", fileName);
            return;
        }
    }

    // Check if there are free inodes available
    if (fs->inode_used >= fs->inode_count)
    {
        printf("No available inodes. File creation failed.\n");
        return;
    }

    // Allocate a new inode
    Inode *newFile = (Inode *)malloc(sizeof(Inode));
    if (!newFile)
    {
        printf("Memory allocation failed for new file.\n");
        return;
    }

    // Initialize the inode properties
    newFile->name = strdup(fileName); // Duplicate the file name
    newFile->is_directory = 0;        
    newFile->file_size = 0;           
    newFile->start_block = -1;        // No block allocated yet
    newFile->block_count = 0;         
    newFile->directory_item_count = 0;
    newFile->directory_items = NULL; 
    newFile->parent = fs->current_directory;

    // Add the new file to the current directory's directory_items array
    size_t newItemCount = fs->current_directory->directory_item_count + 1;
    fs->current_directory->directory_items = (Inode **)realloc(
        fs->current_directory->directory_items, newItemCount * sizeof(Inode *));
    if (!fs->current_directory->directory_items)
    {
        printf("Failed to expand directory items array.\n");
        free(newFile->name);
        free(newFile);
        return;
    }
    fs->current_directory->directory_items[fs->current_directory->directory_item_count] = newFile;
    fs->current_directory->directory_item_count = newItemCount;

    // Add the new inode to the global inodes array
    for (size_t i = 0; i < fs->inode_count; i++)
    {
        if (!fs->inodes[i])
        {
            fs->inodes[i] = newFile;
            break;
        }
    }

    fs->inode_used++;

    printf("File '%s' created successfully.\n", fileName);
}

void rm(FileSystem *fs, const char *filename)
{
    Inode *target = NULL;
    size_t target_index = -1;

    // check if the file exists
    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        if (strcmp(fs->current_directory->directory_items[i]->name, filename) == 0)
        {
            target = fs->current_directory->directory_items[i];
            target_index = i;
            break;
        }
    }
    if (target == NULL)
    {
        printf("File not found: %s\n", filename);
        return;
    }

    // check if the file is a directory
    if (target->is_directory)
    {
        printf("%s is a directory, use rmdir to remove directories.\n", filename);
        return;
    }

    // clear the data blocks(block_bitmap)
    delete_inode(fs, target);

    for (size_t i = target_index; i < fs->current_directory->directory_item_count - 1; i++)
    {
        fs->current_directory->directory_items[i] = fs->current_directory->directory_items[i + 1];
    }
    fs->current_directory->directory_item_count--;

    printf("File %s has been deleted.\n", filename);
}

void cat(FileSystem *fs, const char *filename)
{
    // determine the inode of the file
    Inode *inode = NULL;
    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        if (strcmp(fs->current_directory->directory_items[i]->name, filename) == 0)
        {
            inode = fs->current_directory->directory_items[i];
            break;
        }
    }
    if (inode == NULL)
    {
        printf("File not found: %s\n", filename);
        return;
    }

    // check if the file is a directory
    if (inode->is_directory)
    {
        printf("%s is a directory, not a regular file.\n", filename);
        return;
    }

    // print the content of the file, read data from data blocks
    size_t remaining_size = inode->file_size;
    size_t block_index = inode->start_block;

    while (remaining_size > 0)
    {
        size_t block_size = remaining_size > BLOCK_SIZE ? BLOCK_SIZE : remaining_size;
        printf("%.*s", (int)block_size, &fs->data_blocks[block_index * BLOCK_SIZE]);

        remaining_size -= block_size;
        block_index++;
    }
    printf("\n");
}

void status(FileSystem *fs)
{
    printf("partition size: %zu \n", fs->partition_size);
    printf("total inodes: %zu \n", fs->inode_count);
    printf("used inodes: %zu \n", fs->inode_used);
    printf("total blocks: %zu \n", fs->block_count);
    printf("used blocks: %zu \n", fs->block_used);
    printf("block size: %zu \n", BLOCK_SIZE);
    printf("free space: %zu \n", fs->partition_size - fs->block_used * BLOCK_SIZE);
}

void put(FileSystem *fs, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Failed to open file '%s'.\n", filename);
        return;
    }

    // determine the number of blocks required
    fseek(file, 0, SEEK_END);
    size_t content_size = ftell(file);
    rewind(file);

    size_t required_blocks = (content_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int start_block = -1;
    size_t consecutive_blocks = 0;
    for (size_t i = 0; i < fs->block_count; i++)
    {
        if (fs->block_bitmap[i] == 0)
        {
            if (consecutive_blocks == 0)
            {
                start_block = i;
            }
            consecutive_blocks++;
        }
        else
        {
            consecutive_blocks = 0;
            start_block = -1;
        }

        if (consecutive_blocks == required_blocks)
        {
            break;
        }
    }
    if (consecutive_blocks < required_blocks)
    {
        fclose(file);
        printf("not enough consecutive blocks\n");
        return;
    }

    // allocate valid inode
    int inode_index = -1;
    for (size_t i = 0; i < fs->inode_count; i++)
    {
        if (!fs->inodes[i])
        {
            inode_index = i;
            break;
        }
    }
    if (inode_index == -1)
    {
        fclose(file);
        printf("Not enough space to store the file.\n");
        return;
    }

    // write file content to data blocks
    char buffer[BLOCK_SIZE];
    size_t remaining_size = content_size;
    for (size_t i = 0; i < required_blocks; i++)
    {
        size_t block_offset = start_block + i;
        size_t read_size = (remaining_size > BLOCK_SIZE) ? BLOCK_SIZE : remaining_size;

        fread(buffer, 1, read_size, file);
        memcpy(fs->data_blocks + (block_offset * BLOCK_SIZE), buffer, read_size);
        remaining_size -= read_size;
        fs->block_bitmap[block_offset] = 1;
    }

    fclose(file);

    // create new inode and add it to the current directory
    Inode *new_inode = (Inode *)malloc(sizeof(Inode));
    new_inode->name = strdup(filename);
    new_inode->is_directory = 0;
    new_inode->file_size = content_size;
    new_inode->start_block = start_block;
    new_inode->block_count = required_blocks;
    new_inode->directory_items = NULL;
    new_inode->directory_item_count = 0;

    fs->inodes[inode_index] = new_inode;
    fs->inode_used++;
    fs->block_used += required_blocks;

    if (fs->current_directory->is_directory)
    {
        fs->current_directory->directory_item_count++;
        fs->current_directory->directory_items = realloc(
            fs->current_directory->directory_items,
            fs->current_directory->directory_item_count * sizeof(Inode *));
        fs->current_directory->directory_items[fs->current_directory->directory_item_count - 1] = new_inode;
    }
}

void get(FileSystem *fs, const char *filename)
{
    Inode *target_file = NULL;
    for (size_t i = 0; i < fs->current_directory->directory_item_count; ++i)
    {
        Inode *inode = fs->current_directory->directory_items[i];
        if (!inode->is_directory && strcmp(inode->name, filename) == 0)
        {
            target_file = inode;
            break;
        }
    }
    if (!target_file)
    {
        printf("File '%s' not found in the current directory.\n", filename);
        return;
    }

    // Create dump directory if it doesn't exist
    struct stat st = {0};
    if (stat("dump", &st) == -1)
    {
        if (mkdir("dump") != 0)
        {
            printf("Failed to create 'dump' directory.\n");
            return;
        }
    }

    char filepath[MAX_COMMAND_LENGTH];
    snprintf(filepath, sizeof(filepath), "dump/%s", filename);

    FILE *file = fopen(filepath, "w");
    if (!file)
    {
        printf("Failed to create file '%s' in dump folder.\n", filepath);
        return;
    }

    // from data blocks write to file
    for (size_t i = 0; i < target_file->block_count; ++i)
    {
        int block_index = target_file->start_block + i;
        char *block_data = fs->data_blocks + block_index * BLOCK_SIZE;
        size_t write_size = (i == target_file->block_count - 1)
                                ? target_file->file_size % BLOCK_SIZE
                                : BLOCK_SIZE;
        fwrite(block_data, 1, write_size, file);
    }

    fclose(file);
    printf("File '%s' has been saved to 'dump/%s'.\n", filename, filename);
}
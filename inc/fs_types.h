#ifndef FS_TYPES_H
#define FS_TYPES_H

// for size_t
#include <stddef.h>

#define BLOCK_SIZE 1024
#define INODE_PER_PARTITION 1000

#define MAX_COMMAND_LENGTH 256
#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 256

// Terminal colors for UI
#define COLOR_RESET "\033[0m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_WHITE "\033[0;37m"
#define COLUMN_WIDTH 10

typedef struct Inode
{
    char *name;
    int is_directory;
    size_t file_size;
    int start_block;
    size_t block_count;
    struct Inode *parent;
    struct Inode **directory_items;
    size_t directory_item_count;
} Inode;

typedef struct FileSystem
{
    size_t partition_size;
    size_t block_count;
    size_t block_used;
    char *data_blocks;
    int *block_bitmap;
    size_t inode_count;
    size_t inode_used;
    Inode **inodes;
    Inode *root;
    Inode *current_directory;
} FileSystem;

#endif
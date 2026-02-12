#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCK_SIZE 1024
#define INODE_PER_PARTITION 1000

#define MAX_COMMAND_LENGTH 256
#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 256

// terminal color
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

FileSystem *createFileSystem(size_t size);
void printCurrentPath(Inode *current);
void commandLoop(FileSystem *fs);
void handleCommand(char *command, FileSystem *fs);
void ls(FileSystem *fs);
void cd(FileSystem *fs, const char *path);
void my_mkdir(FileSystem *fs, const char *dirname);
void deleteSubInodes(FileSystem *fs, Inode *inode);
void my_rmdir(FileSystem *fs, const char *dirname);
void put(FileSystem *fs, const char *filename);
void cat(FileSystem *fs, const char *filename);
void get(FileSystem *fs, const char *filename);
void rm(FileSystem *fs, const char *filename);
void touch(FileSystem *fs, const char *fileName);
void status(FileSystem *fs);
void displayHelp();
void saveFileSystem(FileSystem *fs, const char *password);
void loadFileSystem(FileSystem **fs, const char *inputPassword);

FileSystem *createFileSystem(size_t size)
{
    FileSystem *fs = (FileSystem *)malloc(sizeof(FileSystem));
    if (!fs)
    {
        return NULL;
    }

    fs->partition_size = size;
    fs->block_count = size / BLOCK_SIZE;
    fs->block_used = 0;

    fs->data_blocks = (char *)malloc(fs->block_count * BLOCK_SIZE);
    fs->block_bitmap = (int *)calloc(fs->block_count, sizeof(int));

    fs->inode_count = size / INODE_PER_PARTITION;
    fs->inode_used = 0;
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
    fs->inode_used = 1;

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

void commandLoop(FileSystem *fs)
{
    char command[MAX_COMMAND_LENGTH];
    char currentPath[MAX_PATH_LENGTH];

    while (1)
    {
        printCurrentPath(fs->current_directory);
        printf(" $ ");
        scanf("%s", command);
        handleCommand(command, fs);
    }
}

void handleCommand(char *command, FileSystem *fs)
{
    if (strcmp(command, "ls") == 0)
    {
        ls(fs);
    }
    else if (strcmp(command, "cd") == 0)
    {
        char path[MAX_COMMAND_LENGTH];
        scanf("%s", path);
        cd(fs, path);
    }
    else if (strcmp(command, "rm") == 0)
    {
        char filename[MAX_COMMAND_LENGTH];
        scanf("%s", filename);
        rm(fs, filename);
    }
    else if (strcmp(command, "mkdir") == 0)
    {
        char dirname[MAX_COMMAND_LENGTH];
        scanf("%s", dirname);
        my_mkdir(fs, dirname);
    }
    else if (strcmp(command, "rmdir") == 0)
    {
        char dirname[MAX_COMMAND_LENGTH];
        scanf("%s", dirname);
        my_rmdir(fs, dirname);
    }
    else if (strcmp(command, "put") == 0)
    {
        char filename[MAX_COMMAND_LENGTH];
        size_t size;
        scanf("%s", filename);
        put(fs, filename);
    }
    else if (strcmp(command, "cat") == 0)
    {
        char filename[MAX_COMMAND_LENGTH];
        scanf("%s", filename);
        cat(fs, filename);
    }
    else if (strcmp(command, "get") == 0)
    {
        char filename[MAX_COMMAND_LENGTH];
        scanf("%s", filename);
        get(fs, filename);
    }
    else if (strcmp(command, "touch") == 0)
    {
        char filename[MAX_COMMAND_LENGTH];
        scanf("%s", filename);
        touch(fs, filename);
    }
    else if (strcmp(command, "status") == 0)
    {
        status(fs);
    }
    else if (strcmp(command, "exit") == 0)
    {
        char inputPassword[7];
        printf("Please enter the 6-digit password to save and exit: ");
        scanf("%6s", inputPassword);
        saveFileSystem(fs, inputPassword);
        exit(0);
    }
    else if (strcmp(command, "help") == 0)
    {
        displayHelp();
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }
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
        if (inode->is_directory)
        {
            printf("%s%-*s%s", COLOR_BLUE, COLUMN_WIDTH, inode->name, COLOR_RESET);
            count++;
            if (count % 6 == 0)
            {
                printf("\n");
            }
        }
    }
    for (size_t i = 0; i < fs->current_directory->directory_item_count; ++i)
    {
        Inode *inode = fs->current_directory->directory_items[i];
        if (!inode->is_directory)
        {
            printf("%s%-*s%s", COLOR_WHITE, COLUMN_WIDTH, inode->name, COLOR_RESET);
            count++;
            if (count % 6 == 0)
            {
                printf("\n");
            }
        }
    }
    printf("\n");
}

void cd(FileSystem *fs, const char *path)
{
    if (strcmp(path, "..") == 0)
    {
        if (fs->current_directory->parent == NULL)
        {
            printf("Already at root directory.\n");
        }
        else
        {
            fs->current_directory = fs->current_directory->parent;
        }
        return;
    }

    // search for the target directory
    Inode *target_directory = NULL;
    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        if (strcmp(fs->current_directory->directory_items[i]->name, path) == 0)
        {
            target_directory = fs->current_directory->directory_items[i];
            break;
        }
    }
    if (target_directory == NULL)
    {
        printf("Directory %s not found. \n", path);
        return;
    }

    if (!target_directory->is_directory)
    {
        printf("%s is not a directory.\n", path);
        return;
    }

    fs->current_directory = target_directory;
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

    Inode *new_dir = (Inode *)malloc(sizeof(Inode));
    if (!new_dir)
    {
        printf("Failed to allocate memory for directory.\n");
        return;
    }

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

    if (fs->current_directory->is_directory)
    {
        fs->current_directory->directory_item_count++;
        fs->current_directory->directory_items = realloc(fs->current_directory->directory_items, fs->current_directory->directory_item_count * sizeof(Inode *));
        fs->current_directory->directory_items[fs->current_directory->directory_item_count - 1] = new_dir;
    }
}

void deleteSubInodes(FileSystem *fs, Inode *inode)
{
    for (size_t i = 0; i < inode->directory_item_count; i++)
    {
        Inode *child = inode->directory_items[i];
        if (child->is_directory)
        {
            deleteSubInodes(fs, child);
        }

        if (!child->is_directory)
        {
            for (size_t j = 0; j < child->block_count; j++)
            {
                int block_index = child->start_block + i;
                fs->block_bitmap[block_index] = 0;
            }
            fs->block_used -= child->block_count;
        }
        fs->inode_used--;

        for (size_t k = 0; k < fs->inode_count; k++)
        {
            if (fs->inodes[k] == child)
            {
                fs->inodes[k] = NULL;
                break;
            }
        }
        free(child->name);
        free(child);
    }
    free(inode->directory_items);
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

    deleteSubInodes(fs, target);

    for (size_t i = target_index; i < fs->current_directory->directory_item_count - 1; i++)
    {
        fs->current_directory->directory_items[i] = fs->current_directory->directory_items[i + 1];
    }
    fs->current_directory->directory_item_count--;

    fs->inode_used--;
    for (size_t i = 0; i < fs->inode_count; i++)
    {
        if (fs->inodes[i] == target)
        {
            fs->inodes[i] = NULL;
            break;
        }
    }

    free(target->name);
    free(target);

    printf("Directory '%s' and its contents have been removed.\n", dirname);
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
    size_t bytes_read = 0;
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

    // 檢查或建立 dump 資料夾
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

void rm(FileSystem *fs, const char *filename)
{
    // check if the file exists
    Inode *inode_to_delete = NULL;
    size_t inode_index = -1;
    for (size_t i = 0; i < fs->current_directory->directory_item_count; i++)
    {
        if (strcmp(fs->current_directory->directory_items[i]->name, filename) == 0)
        {
            inode_to_delete = fs->current_directory->directory_items[i];
            inode_index = i;
            break;
        }
    }
    if (inode_to_delete == NULL)
    {
        printf("File not found: %s\n", filename);
        return;
    }

    // check if the file is a directory
    if (inode_to_delete->is_directory)
    {
        printf("%s is a directory, use rmdir to remove directories.\n", filename);
        return;
    }

    // clear the data blocks(block_bitmap)
    for (size_t i = 0; i < inode_to_delete->block_count; i++)
    {
        size_t block_index = inode_to_delete->start_block + i;
        fs->block_bitmap[block_index] = 0;
    }

    fs->block_used -= inode_to_delete->block_count;
    free(inode_to_delete->name);
    free(inode_to_delete);
    fs->inodes[inode_index] = NULL;

    for (size_t i = inode_index; i < fs->current_directory->directory_item_count - 1; i++)
    {
        fs->current_directory->directory_items[i] = fs->current_directory->directory_items[i + 1];
    }

    fs->current_directory->directory_item_count--;
    fs->inode_used--;

    printf("File %s has been deleted.\n", filename);
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
    newFile->is_directory = 0;        // It is a file
    newFile->file_size = 0;           // Initial size is 0
    newFile->start_block = -1;        // No block allocated yet
    newFile->block_count = 0;         // No blocks used
    newFile->directory_item_count = 0;
    newFile->directory_items = NULL; // Not a directory
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
        if (!fs->inodes[i]) // Find a free slot
        {
            fs->inodes[i] = newFile;
            break;
        }
    }

    // Update filesystem metadata
    fs->inode_used++;

    printf("File '%s' created successfully.\n", fileName);
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

void displayHelp()
{
    printf("List of commands:\n");
    printf("  ls       - List files and directories\n");
    printf("  cd       - Change directory\n");
    printf("  mkdir    - Make directory\n");
    printf("  rmdir    - Remove directory\n");
    printf("  put      - Put file into the space\n");
    printf("  cat      - Show content\n");
    printf("  get      - Get file from the space\n");
    printf("  rm       - Remove file\n");
    printf("  status   - Show status of space\n");
    printf("  help     - Show help\n");
    printf("  exit     - Exit and store img\n");
}

void saveInode(FILE *file, Inode *inode)
{
    // Save the basic properties of the inode
    size_t name_length = strlen(inode->name) + 1;
    fwrite(&name_length, sizeof(size_t), 1, file);        // Save name length
    fwrite(inode->name, sizeof(char), name_length, file); // Save name
    fwrite(&inode->is_directory, sizeof(int), 1, file);
    fwrite(&inode->file_size, sizeof(size_t), 1, file);
    fwrite(&inode->start_block, sizeof(int), 1, file);
    fwrite(&inode->block_count, sizeof(size_t), 1, file);
    fwrite(&inode->directory_item_count, sizeof(size_t), 1, file);

    // Save directory items if it is a directory
    if (inode->is_directory && inode->directory_items != NULL)
    {
        for (size_t i = 0; i < inode->directory_item_count; i++)
        {
            saveInode(file, inode->directory_items[i]);
        }
    }
}

void saveFileSystem(FileSystem *fs, const char *password)
{
    FILE *file = fopen("filesystem.dump", "wb");
    if (!file)
    {
        printf("Failed to create dump file.\n");
        return;
    }

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
    saveInode(file, fs->root);

    // Save the data blocks
    fwrite(fs->data_blocks, BLOCK_SIZE, fs->block_count, file);

    fclose(file);
    printf("File system has been saved to 'filesystem.dump' with password.\n");
}

void loadInode(FILE *file, Inode **inode, Inode *parent)
{
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
            loadInode(file, &(*inode)->directory_items[i], *inode);
        }
    }
    else
    {
        // If it is not a directory, set directory_items to NULL
        (*inode)->directory_items = NULL;
    }
}

void loadFileSystem(FileSystem **fs, const char *inputPassword)
{
    FILE *file = fopen("filesystem.dump", "rb");
    if (!file)
    {
        printf("Failed to open dump file.\n");
        return;
    }

    // Read and verify the password
    char storedPassword[7] = {0}; // Extra space for null terminator
    fread(storedPassword, sizeof(char), 6, file);

    if (strncmp(storedPassword, inputPassword, 6) != 0)
    {
        printf("Password verification failed. Unable to load the file system.\n");
        fclose(file);
        return;
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
    loadInode(file, &(*fs)->root, NULL);
    (*fs)->inodes[0] = (*fs)->root;
    (*fs)->current_directory = (*fs)->root;

    // Load the data blocks
    (*fs)->data_blocks = (char *)malloc((*fs)->block_count * BLOCK_SIZE);
    fread((*fs)->data_blocks, BLOCK_SIZE, (*fs)->block_count, file);

    fclose(file);
    printf("File system has been loaded from 'filesystem.dump'.\n");
}

int main()
{
    int option;

    FileSystem *fileSystem = NULL;

    printf("Options:\n");
    printf(" 1. Load from file\n");
    printf(" 2. Create new partition in memory\n");
    printf("Choose an option: ");
    scanf("%d", &option);

    if (option == 2)
    {
        size_t fileSystemSize;

        printf("Input size of a new partition (example: 102400 2048000)\n");
        printf("partition size = ");
        scanf("%zu", &fileSystemSize);

        fileSystem = createFileSystem(fileSystemSize);
        if (!fileSystem)
        {
            printf("Failed to create partition.\n");
            return 1;
        }
        printf("Make new partition successful!\n");

        commandLoop(fileSystem);
    }
    else if (option == 1)
    {
        char password[7];
        printf("Please enter the 6-digit password: ");
        scanf("%6s", password);

        loadFileSystem(&fileSystem, password);
        commandLoop(fileSystem);
    }
    else
    {
        printf("Invalid option.\n");
    }

    return 0;
}
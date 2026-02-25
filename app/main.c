#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.h"

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

void handleCommand(char *command, FileSystem *fs)
{
    if (strcmp(command, "ls") == 0)
        ls(fs);
    else if (strcmp(command, "cd") == 0)
    {
        char path[MAX_COMMAND_LENGTH];
        scanf("%s", path);
        cd(fs, path);
    }
    else if (strcmp(command, "mkdir") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        my_mkdir(fs, name);
    }
    else if (strcmp(command, "rmdir") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        my_rmdir(fs, name);
    }
    else if (strcmp(command, "put") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        put(fs, name);
    }
    else if (strcmp(command, "cat") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        cat(fs, name);
    }
    else if (strcmp(command, "get") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        get(fs, name);
    }
    else if (strcmp(command, "rm") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        rm(fs, name);
    }
    else if (strcmp(command, "touch") == 0)
    {
        char name[MAX_NAME_LENGTH];
        scanf("%s", name);
        touch(fs, name);
    }
    else if (strcmp(command, "status") == 0)
        status(fs);
    else if (strcmp(command, "help") == 0)
        displayHelp();
    else if (strcmp(command, "exit") == 0)
    {
        char pwd[7];
        printf("Enter 6-digit password to save: ");
        scanf("%6s", pwd);
        saveFileSystem(fs, pwd);
        exit(0);
    }
    else
        printf("Unknown command: %s\n", command);
}

void commandLoop(FileSystem *fs)
{
    char command[MAX_COMMAND_LENGTH];

    while (1)
    {
        printCurrentPath(fs->current_directory);
        printf(" $ ");
        if (scanf("%s", command) == EOF)
            break;
        handleCommand(command, fs);
    }
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

    // write
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
    // read
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
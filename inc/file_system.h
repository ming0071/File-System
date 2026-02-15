#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "fs_types.h"

// Core Lifecycle
FileSystem *createFileSystem(size_t size);
// void freeFileSystem(FileSyste。m *fs); // 建議新增：釋放所有資源

// File Operations
void my_mkdir(FileSystem *fs, const char *dirname);
void my_rmdir(FileSystem *fs, const char *dirname);
void put(FileSystem *fs, const char *filename);
void get(FileSystem *fs, const char *filename);
void cat(FileSystem *fs, const char *filename);
void rm(FileSystem *fs, const char *filename);
void touch(FileSystem *fs, const char *fileName);

// Navigation & Info
void ls(FileSystem *fs);
void cd(FileSystem *fs, const char *path);
void status(FileSystem *fs);
void printCurrentPath(Inode *current);

// Persistence (I/O)
void saveFileSystem(FileSystem *fs, const char *password);
void loadFileSystem(FileSystem **fs, const char *inputPassword);

#endif
CC = gcc
CFLAGS = -Wall -Wextra -I./inc -g
# 先暫時移除 -fsanitize=address 以確保 Windows GCC 能順利連結

# Paths
SRC_DIR = src
APP_DIR = app
OBJ_DIR = obj
BIN_DIR = bin

# Targets
TARGET = $(BIN_DIR)/fs_sim.exe

# Files
SRCS = $(SRC_DIR)/file_system_core.c $(SRC_DIR)/file_system_io.c
APP_SRCS = $(APP_DIR)/main.c
OBJS = $(OBJ_DIR)/file_system_core.o $(OBJ_DIR)/file_system_io.o
APP_OBJS = $(OBJ_DIR)/main.o

all: $(TARGET)

$(TARGET): $(OBJS) $(APP_OBJS)
	$(CC) $(OBJS) $(APP_OBJS) -o $@

# 這裡移除了 @mkdir，因為你已經手動建好了
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(APP_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /q $(OBJ_DIR)\*.o $(BIN_DIR)\*.exe
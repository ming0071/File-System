# 使用 "mingw32-make" 進行編譯

# 編譯器設定
CC = gcc
# 編譯旗標 (-I 指定標頭檔目錄，-g 為了除錯，-Wall 顯示所有警告)
CFLAGS = -Wall -Wextra -I./inc -g

# 目錄路徑定義
SRC_DIR = src
APP_DIR = app
OBJ_DIR = obj
BIN_DIR = bin

# 自動搜尋來源檔案 (找出 src/ 和 app/ 底下所有的 .c 檔案)
SRCS_CORE = $(wildcard $(SRC_DIR)/*.c)
SRCS_APP = $(wildcard $(APP_DIR)/*.c)

# 自動轉換路徑 (將 .c 檔名轉換為存放於 obj/ 底下的 .o 檔名)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS_CORE))
APP_OBJS = $(patsubst $(APP_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS_APP))
ALL_OBJS = $(OBJS) $(APP_OBJS)

# 最終執行檔路徑
TARGET = $(BIN_DIR)/fs_sim.exe

# 預設目標 (直接執行 mingw32-make 時會跑這項)
all: $(TARGET)

# 連結規則：將所有 .o 檔案結合為一個 .exe 執行檔
$(TARGET): $(ALL_OBJS)
	$(CC) $(ALL_OBJS) -o $@

# 編譯規則 1：將 src/ 下的 .c 編譯成 obj/ 下的 .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 編譯規則 2：將 app/ 下的 .c 編譯成 obj/ 下的 .o
$(OBJ_DIR)/%.o: $(APP_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理規則：刪除所有的編譯產物 (適用於 Windows CMD)
clean:
	@if exist $(OBJ_DIR) del /q $(OBJ_DIR)\*.o
	@if exist $(BIN_DIR) del /q $(BIN_DIR)\*.exe
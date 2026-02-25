# File System Simulator

本專案為 **進階 C 語言課程** 的期末專案。實作一個基於記憶體模擬的檔案系統，模擬現代作業系統中磁碟分區、檔案管理與資料持久化的核心邏輯。

## 專案概念
本專案透過分配一塊連續的記憶體空間來模擬硬體磁碟。使用者可以在此空間內進行完整的檔案操作。系統支援將記憶體中的狀態保存為二進位映像檔（.dump），並在下次啟動時透過密碼驗證還原。

## 系統架構設計
採用經典的 Inode-based 結構，透過 FileSystem、Inode、Blocks 三個層次維護系統運作，精確管理 inodes 分配、block_bitmap 狀態、data_blocks 儲存空間
1. FileSystem 全域管理中心：
* 維護全域狀態，包含分區大小、Inode 使用率與區塊佔用統計。

2. Inode 樹狀結構：
* 透過 parent 指標與 directory_items 動態陣列維護目錄階層，實現高效的路徑導航與遞迴清理。
* Inode 代表檔目錄或檔案，每個 Inode 紀錄檔案名稱、大小、類型及 start_block(如果是檔案)。

3. Blocks 存下「檔案」二進位數據
* 目錄資訊完全由 inodes 結構維護，不額外占用 data_blocks。
* 檔案資訊由 Block Bitmap 管理空間分配，data_blocks 存存檔案資料。

## 功能說明
本系統提供 CLI 互動介面，包含以下功能：
* **檔案/目錄操作**：`ls` (列出清單)、`cd` (切換路徑)、`mkdir/rmdir` (目錄增刪)、`touch/rm` (檔案增刪)。
* **檢視與管理**：`cat` (讀取內容)、`status` (顯示磁碟與 Inode 使用狀況)。
* **外部整合**：`put` (從真實系統匯入檔案)、`get` (匯出檔案至真實系統)。
* **安全機制**：`exit` (驗證 6 位數密碼並儲存狀態)。

## 核心技術與實現
* **Inode 索引架構**：每個檔案與目錄均配有一個 Inode 結構，紀錄檔案元數據（Metadata），如名稱、大小、類型及所佔用的區塊索引。
* **連續空間分配**：檔案寫入時搜尋 Bitmap 中的連續空閒區塊，確保數據在磁碟上的物理連續性，優化順序讀寫。
* **遞迴資源回收**：實作深度優先搜尋（DFS）遞迴邏輯。刪除目錄時，系統會先序遍歷子項，確保所有巢狀目錄、檔案及其佔用的物理區塊 (Bitmap) 與 Inode 被徹底釋放。
* **資料持久化**：實作自定義序列化機制，將複雜的記憶體樹狀結構轉為線性二進位檔。儲存順序為：
1. 安全驗證密碼
2. FileSystem 全域狀態
3. Block Bitmap 映像
4. Inode Tree (DFS Pre-order)
5. Raw Data Blocks

## 編譯、執行與測試指令

### 1. 編譯專案 (Makefile)
```powershell
mingw32-make clean
mingw32-make
```

### 2. 啟動程式
```powershell
./bin/fs_sim.exe
```

### 3. 自動化功能測試
```powershell
./run_test.ps1
```

確認測試檔案的實際輸出
```powershell
cmd /c "type tests\test_file.txt | bin\fs_sim.exe"
```
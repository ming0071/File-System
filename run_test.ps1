# 1. 環境設定
$scriptFolder = "tests"
$exeFile = "bin\fs_sim.exe"
$testFiles = Get-ChildItem -Path $scriptFolder -Filter *.txt

Write-Host "=== Starting Functional Verification ===" -ForegroundColor Cyan

foreach ($file in $testFiles) {
    Write-Host "Running: $($file.Name)" -NoNewline
    
    # 執行並獲取輸出
    $actual = cmd /c "type $($file.FullName) | $exeFile" | Out-String
    $pass = $false

    # 2. 針對不同檔案執行特定檢查邏輯
    switch ($file.Name) {
        "test_mkdir.txt" {
            # 檢查是否成功建立目錄並在 ls 中顯示
            if ($actual.Contains("Directory 'folder1' created") -and $actual.Contains("folder1")) { $pass = $true }
        }
        "test_rm.txt" {
            # 檢查刪除後，ls 是否不再顯示該檔案
            if ($actual.Contains("deleted successfully") -and (-not $actual.Contains("old_file.txt"))) { $pass = $true }
        }
        default {
            # 萬用基本檢查
            if ($actual.Contains("successful")) { $pass = $true }
        }
    }

    # 3. 顯示結果
    if ($pass) {
        Write-Host " -> [PASS]" -ForegroundColor Green
    } else {
        Write-Host " -> [FAIL]" -ForegroundColor Red
        Write-Host "--- Error Diagnostic ---"
        Write-Output $actual
    }
}

Write-Host "=== Verification Complete ===" -ForegroundColor Cyan
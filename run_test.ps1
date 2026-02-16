# Setup paths
$scriptFile = "tests\test_script.txt"
$exeFile = "bin\fs_sim.exe"

# Basic check
if (-not (Test-Path $scriptFile)) { echo "Missing script"; exit }
if (-not (Test-Path $exeFile)) { echo "Missing exe"; exit }

# Run execution
$actual = cmd /c "type $scriptFile | $exeFile" | Out-String

# Logic validation
$c1 = $actual.Contains("partition size: 1024000")
$c2 = $actual.Contains("used inodes: 3")
$c3 = $actual.Contains("free space: 1022976")
$c4 = $actual.Contains("File system has been saved")

# Output result
Write-Host "------------------------------"
if ($c1 -and $c2 -and $c3 -and $c4) {
    Write-Host "TEST PASSED" -ForegroundColor Green
} else {
    Write-Host "TEST FAILED" -ForegroundColor Red
    Write-Output $actual
}
Write-Host "------------------------------"
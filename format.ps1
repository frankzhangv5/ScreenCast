# 格式化src和include目录的C++代码
$total = 0

Write-Host "Starting code formatting..." -ForegroundColor Cyan

Get-ChildItem -Path .\src\,.\include\ -Include *.h, *.cpp -Recurse | ForEach-Object {
    $file = $_.FullName
    try {
        clang-format -i --style=file --verbose "$file"
        Write-Host "Formatted: $file" -ForegroundColor Green
        $total++
    }
    catch {
        Write-Host "Error formatting: $file" -ForegroundColor Red
    }
}

Write-Host "\nFormatting completed!" -ForegroundColor Cyan
Write-Host "Total files processed: $total" -ForegroundColor Yellow
Write-Host "Detailed logs available in clang-format output" -ForegroundColor Gray
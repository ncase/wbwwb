$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $ProjectRoot
try {
    $TestFiles = Get-ChildItem -Path '.\tests' -Filter '*.test.js' | Sort-Object Name
    foreach ($TestFile in $TestFiles) {
        Write-Host "`n== $($TestFile.Name) ==" -ForegroundColor Cyan
        & node $TestFile.FullName
        if ($LASTEXITCODE -ne 0) {
            throw "Test failed: $($TestFile.Name)"
        }
    }

    Write-Host "`n== JavaScript syntax checks ==" -ForegroundColor Cyan
    $JsFiles = Get-ChildItem -Path '.\js', '.\tests' -Filter '*.js' -Recurse |
        Where-Object { $_.FullName -notmatch '[\\/]js[\\/]lib[\\/]' }
    foreach ($JsFile in $JsFiles) {
        & node --check $JsFile.FullName
        if ($LASTEXITCODE -ne 0) {
            throw "Syntax check failed: $($JsFile.FullName)"
        }
    }

    Write-Host "`nPASS: $($TestFiles.Count) test suites and $($JsFiles.Count) JavaScript syntax checks." -ForegroundColor Green
}
finally {
    Pop-Location
}

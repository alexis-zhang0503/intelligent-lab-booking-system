param([string]$OutDir = "$PSScriptRoot\dist")
$ErrorActionPreference = 'Stop'
[IO.File]::WriteAllBytes((Join-Path $PSScriptRoot 'assets/lab-logo.ico'), [Convert]::FromBase64String((Get-Content (Join-Path $PSScriptRoot 'assets/lab-logo.ico.b64') -Raw)))
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run from an x64 Native Tools Command Prompt for Visual Studio with the Windows SDK.'
}
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
Push-Location $OutDir
try {
    # No browser SDK, runtime download or C++ bridge. All translation units are C.
    Push-Location $PSScriptRoot
    try {
        & rc.exe /nologo /fo "$OutDir\app.res" app.rc
        if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
    } finally { Pop-Location }
    # Business layer: the three presentation modules plus shared helpers. No Windows APIs; also linked by the tests.
    $client = @(
        "$PSScriptRoot\src\1_cloud_data.c",
        "$PSScriptRoot\src\2_booking.c",
        "$PSScriptRoot\src\3_ai_agent.c",
        "$PSScriptRoot\src\other\client_common.c",
        "$PSScriptRoot\src\other\client_extras.c",
        "$PSScriptRoot\vendor\cJSON.c"
    )
    & cl.exe /nologo /TC /std:c11 /utf-8 /O2 /MT /W4 /DUNICODE /D_UNICODE "$PSScriptRoot\src\other\main.c" "$PSScriptRoot\src\other\win_http.c" @client /Fe:LabReservation.exe /link app.res /SUBSYSTEM:WINDOWS ole32.lib shell32.lib user32.lib gdi32.lib winhttp.lib comctl32.lib comdlg32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Native C desktop compilation failed.' }
    & cl.exe /nologo /TC /std:c11 /utf-8 /W4 "$PSScriptRoot\tests\client_test.c" @client /Fe:ClientCheck.exe
    if ($LASTEXITCODE -ne 0) { throw 'C client test compilation failed.' }
    & .\ClientCheck.exe
    if ($LASTEXITCODE -ne 0) { throw 'C client tests failed.' }
    Copy-Item "$PSScriptRoot\README.md" . -Force
    Copy-Item "$PSScriptRoot\vendor\CJSON-LICENSE.txt" . -Force
    Remove-Item *.obj, *.res, ClientCheck.exe -ErrorAction SilentlyContinue
    Write-Host "Built native C desktop at $OutDir"
} finally { Pop-Location }

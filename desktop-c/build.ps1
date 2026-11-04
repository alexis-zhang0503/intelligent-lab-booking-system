param([string]$OutDir = "$PSScriptRoot\dist")
$ErrorActionPreference = 'Stop'
[IO.File]::WriteAllBytes((Join-Path $PSScriptRoot 'assets/lab-logo.ico'), [Convert]::FromBase64String((Get-Content (Join-Path $PSScriptRoot 'assets/lab-logo.ico.b64') -Raw)))
$SdkVersion = '1.0.2903.40'
$SdkHash = 'ef128016dd1e51c59178c827ed5b8aa3322c57afa8675d930f8109505542ad74'
$SdkRoot = Join-Path $PSScriptRoot 'vendor-sdk'
$Package = Join-Path $PSScriptRoot 'webview2-sdk.zip'
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run from an x64 Native Tools Command Prompt for Visual Studio (with the Windows SDK).'
}
if (-not (Test-Path "$SdkRoot\build\native\include\WebView2.h")) {
    Invoke-WebRequest "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/$SdkVersion/microsoft.web.webview2.$SdkVersion.nupkg" -OutFile $Package
    if ((Get-FileHash $Package -Algorithm SHA256).Hash.ToLowerInvariant() -ne $SdkHash) {
        throw 'WebView2 SDK checksum mismatch.'
    }
    Expand-Archive $Package -DestinationPath $SdkRoot -Force
}
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
Push-Location $OutDir
try {
    Push-Location $PSScriptRoot
    try {
        & rc.exe /nologo /fo "$OutDir\app.res" app.rc
        if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
    } finally { Pop-Location }
    & cl.exe /nologo /TC /std:c11 /utf-8 /O2 /MT /W4 /DUNICODE /D_UNICODE "/I$SdkRoot\build\native\include" "$PSScriptRoot\src\main.c" app.res /Fe:LabReservation.exe /link /SUBSYSTEM:WINDOWS ole32.lib shell32.lib uuid.lib user32.lib gdi32.lib
    if ($LASTEXITCODE -ne 0) { throw 'C desktop compilation failed.' }
    & cl.exe /nologo /TC /std:c11 /W4 "$PSScriptRoot\tests\site_policy_test.c" /Fe:SitePolicyCheck.exe
    if ($LASTEXITCODE -ne 0) { throw 'C policy test compilation failed.' }
    & .\SitePolicyCheck.exe
    if ($LASTEXITCODE -ne 0) { throw 'C policy test failed.' }
    Copy-Item "$SdkRoot\build\native\x64\WebView2Loader.dll" . -Force
    Get-ChildItem $SdkRoot -File | Where-Object Name -Match 'license|notice' | ForEach-Object {
        Copy-Item $_.FullName . -Force
    }
    Copy-Item "$PSScriptRoot\README.md" . -Force
    Remove-Item *.obj, *.res, SitePolicyCheck.exe -ErrorAction SilentlyContinue
    Write-Host "Built C desktop at $OutDir"
} finally { Pop-Location }

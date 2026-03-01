# AI Shell Portable Build Script
# This script builds the project in Release mode and packages it into a ZIP file.

$ProjectRoot = Get-Location
$BuildDir = "$ProjectRoot\build_release"
$DistDir = "$ProjectRoot\dist"
$ZipName = "cmdAI-Portable.zip"

write-host "🚀 Starting Portable Build Process..." -ForegroundColor Cyan

# 1. Clean and Create directories
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
New-Item -ItemType Directory -Path $BuildDir
New-Item -ItemType Directory -Path $DistDir

# 2. Configure and Build
write-host "🛠️ Configuring CMake (Release)..." -ForegroundColor Yellow
cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=Release

write-host "🏗️ Building AIHollowShell..." -ForegroundColor Yellow
cmake --build $BuildDir --config Release --target AIHollowShell

# 3. Identify the output binary folder
$BinaryFolder = "$BuildDir\Release"
if (-not (Test-Path "$BinaryFolder\AIHollowShell.exe")) {
    $BinaryFolder = $BuildDir # Fallback for some generators
}

# 4. Copy essentials to Dist folder
write-host "📦 Collecting files for distribution..." -ForegroundColor Yellow
Copy-Item "$BinaryFolder\AIHollowShell.exe" "$DistDir\cmdAI.exe"
Get-ChildItem "$BinaryFolder\*.dll" | Copy-Item -Destination $DistDir

# Handle Models - We'll create a 'models' folder in dist
$DistModels = "$DistDir\models"
New-Item -ItemType Directory -Path $DistModels

# Look for the smallest model available in the local models folder
$LocalModels = "$ProjectRoot\models"
if (Test-Path $LocalModels) {
    write-host "🤖 Looking for models to include..." -ForegroundColor Yellow
    # Sort by size and take the smallest .gguf
    $SmallestModel = Get-ChildItem "$LocalModels\*.gguf" | Sort-Object Length | Select-Object -First 1
    if ($null -ne $SmallestModel) {
        write-host "✅ Including model: $($SmallestModel.Name) ($([Math]::Round($SmallestModel.Length/1MB, 2)) MB)" -ForegroundColor Green
        Copy-Item $SmallestModel.FullName -Destination $DistModels
    } else {
        write-host "⚠️ No model found in $LocalModels. User will need to download one." -ForegroundColor Red
    }
}

# 5. Create ZIP
write-host "🤐 Creating ZIP archive..." -ForegroundColor Yellow
if (Test-Path "$ProjectRoot\$ZipName") { Remove-Item "$ProjectRoot\$ZipName" }

# Use Compress-Archive (Native PowerShell)
Compress-Archive -Path "$DistDir\*" -DestinationPath "$ProjectRoot\$ZipName" -Force

write-host "✨ Done! Portable package created: $ZipName" -ForegroundColor Green
write-host "📁 Location: $ProjectRoot\$ZipName" -ForegroundColor Cyan

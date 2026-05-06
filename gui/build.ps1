# build.ps1 — Compila simulacion_gui via MSYS2 bash (ucrt64)
#
# Uso:
#   cd gui
#   .\build.ps1          <- compila
#   .\build.ps1 -Run     <- compila y ejecuta
#
# Requiere MSYS2 en C:\msys64 con los paquetes:
#   mingw-w64-ucrt-x86_64-gcc
#   mingw-w64-ucrt-x86_64-raylib
# (el script los instala automaticamente si faltan)

param([switch]$Run)

$msys2   = "C:\msys64"
$bash    = "$msys2\usr\bin\bash.exe"
$tmpDir  = "$msys2\tmp"
$srcOrig = "$PSScriptRoot\simulacion_gui.c"
$outOrig = "$PSScriptRoot\simulacion_gui.exe"
$srcTmp  = "$tmpDir\simulacion_gui.c"
$outTmp  = "$tmpDir\simulacion_gui.exe"

if (-not (Test-Path $bash)) {
    Write-Host "ERROR: MSYS2 no encontrado en $msys2" -ForegroundColor Red
    Write-Host "Instala desde https://www.msys2.org y luego:" -ForegroundColor Yellow
    Write-Host "  pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib" -ForegroundColor Yellow
    exit 1
}

# Instalar dependencias si faltan
$gccBin = "$msys2\ucrt64\bin\gcc.exe"
if (-not (Test-Path $gccBin)) {
    Write-Host "Instalando gcc y raylib (1-2 min)..." -ForegroundColor Cyan
    cmd /c "$msys2\usr\bin\pacman.exe -S --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib 2>&1"
}

# Copiar fuente al tmp de MSYS2 (sin espacios en ruta)
Copy-Item $srcOrig $srcTmp -Force

# Script bash de compilacion
$script = @"
export PATH=/ucrt64/bin:`$PATH
gcc -I/ucrt64/include -Wall -O2 /tmp/simulacion_gui.c \
    -L/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows \
    -o /tmp/simulacion_gui.exe
echo "gcc_exit:`$?"
"@
[System.IO.File]::WriteAllText("$tmpDir\build_gui.sh", $script, [System.Text.Encoding]::ASCII)

Write-Host "Compilando..." -ForegroundColor Cyan
$output = cmd /c "`"$bash`" --login /tmp/build_gui.sh 2>&1"
Write-Host $output

if ((Test-Path $outTmp) -and ($output -match "gcc_exit:0")) {
    Copy-Item $outTmp $outOrig -Force
    # Copiar DLLs necesarias junto al exe
    foreach ($dll in @("libraylib.dll", "glfw3.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll")) {
        $dllSrc = "$msys2\ucrt64\bin\$dll"
        if ((Test-Path $dllSrc) -and (-not (Test-Path "$PSScriptRoot\$dll"))) {
            Copy-Item $dllSrc "$PSScriptRoot\$dll"
        }
    }
    Write-Host "Compilado: simulacion_gui.exe" -ForegroundColor Green
    if ($Run) {
        Write-Host "Ejecutando..." -ForegroundColor Cyan
        Start-Process -FilePath $outOrig
    } else {
        Write-Host "Para ejecutar: .\simulacion_gui.exe" -ForegroundColor Cyan
    }
} else {
    Write-Host "Error de compilacion." -ForegroundColor Red
    exit 1
}

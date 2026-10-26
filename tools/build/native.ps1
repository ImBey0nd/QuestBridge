$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$compiler = Join-Path $root '.tools/w64devkit/bin'
if (Test-Path $compiler) {
 $env:PATH = "$compiler;$env:PATH"
 cmake -S $root -B "$root/build" -G 'MinGW Makefiles' "-DCMAKE_MAKE_PROGRAM=$compiler/mingw32-make.exe"
} else { cmake -S $root -B "$root/build" }
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build "$root/build" --config Debug
if ($LASTEXITCODE) { exit $LASTEXITCODE }
ctest --test-dir "$root/build" -C Debug --output-on-failure
exit $LASTEXITCODE

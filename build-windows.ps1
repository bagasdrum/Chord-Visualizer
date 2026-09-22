$ErrorActionPreference = 'Stop'

Write-Host '=== ChordScope Windows x64 build ==='

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake is required for a local Windows build. For a zero-install build, use GitHub Actions instead.'
}

if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    throw 'Node.js/npm is required for the React UI. For a zero-install build, use GitHub Actions instead.'
}

npm install
npm run build
npm run lint

cmake -S . -B build -G 'Visual Studio 17 2022' -A x64
cmake --build build --config Release --parallel

New-Item -ItemType Directory -Force -Path release | Out-Null
$vst3 = Get-ChildItem -Path build -Recurse -Directory -Filter 'ChordScope.vst3' | Select-Object -First 1
if (-not $vst3) { throw 'ChordScope.vst3 was not found after build.' }

Remove-Item release/ChordScope.vst3 -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item $vst3.FullName release/ChordScope.vst3 -Recurse -Force
Copy-Item dist release/web-ui -Recurse -Force

Write-Host "Build complete: $((Resolve-Path release/ChordScope.vst3).Path)"

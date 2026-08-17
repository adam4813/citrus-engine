# TODO: Add support for linting only staged files
# This would allow running: .\scripts\lint-code.ps1 -Staged

# If git bash exists use it to run the make command.
# The make command is the source of truth, the powershell script is a fallback.

if (Test-Path "$env:ProgramFiles\Git\bin\bash.exe") {
    & "$env:ProgramFiles\Git\bin\bash.exe" -c "make lint"
}
else {
    Write-Host "Git bash not found, invoking clang-tidy directly"
    if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
        Write-Host "Python could not be found"
        exit 1
    }
    if (-not (Get-Command clang-tidy -ErrorAction SilentlyContinue)) {
        Write-Host "clang-tidy could not be found"
        exit 1
    }
    & python (Get-Command run-clang-tidy).Source -p ./build/native/ -extra-arg="--experimental-modules-support" -extra-arg="-fprebuilt-module-path=./build/native/src/engine/CMakeFiles/engine-core.dir/Debug"
}
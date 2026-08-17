# TODO: Add support for formatting only staged files
# This would allow running: .\scripts\format-code.ps1 -Staged

# If git bash exists use it to run the make command.
# The make command is the source of truth, the powershell script is a fallback.

if (Test-Path "$env:ProgramFiles\Git\bin\bash.exe") {
    & "$env:ProgramFiles\Git\bin\bash.exe" -c "make format"
}
else {
    Write-Host "Git bash not found, invoking clang-format directly"
    if (-not (Get-Command clang-format -ErrorAction SilentlyContinue)) {
        Write-Host "clang-format not found, please install it and add it to your PATH."
        exit 1
    }
    & clang-format -i -style=file $(git ls-files --exclude-standard | Where-Object { $_ -match '\.(c|cpp|h|hpp|cppm)$' })
}
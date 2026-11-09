@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Merge-UnrealFilters.ps1" -ProjectRoot "%~dp0."
set "filterExitCode=%ERRORLEVEL%"
if not "%filterExitCode%"=="0" echo Filter update failed.
exit /b %filterExitCode%

@echo off
setlocal

set "ROOT=%~dp0"
set "GUI_EXE=%ROOT%build\fcasher.exe"
set "CLI_EXE=%ROOT%build\fcasher_cli.exe"

if not exist "%GUI_EXE%" (
    echo Fcasher GUI executable was not found:
    echo   %GUI_EXE%
    echo.
    echo Build the project first or keep this script next to the repository root.
    echo.
    pause
    exit /b 1
)

if "%~1"=="" (
    start "" "%GUI_EXE%"
) else (
    if not exist "%CLI_EXE%" (
        echo Fcasher CLI executable was not found:
        echo   %CLI_EXE%
        echo.
        pause
        exit /b 1
    )
    "%CLI_EXE%" %*
    echo.
    pause
)

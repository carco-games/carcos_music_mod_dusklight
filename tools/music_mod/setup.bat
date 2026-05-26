@echo off

setlocal enabledelayedexpansion

cd /d "%~dp0"

set DECOMP_PATH=%~dp0..\..
REM Remove trailing backslash from DECOMP_PATH
if "%DECOMP_PATH:~-1%"=="\" set DECOMP_PATH=%DECOMP_PATH:~0,-1%

REM Load environment variables from .env file if it exists
if exist "%DECOMP_PATH%\.env" (
    echo Loading configuration from .env...
    for /f "usebackq tokens=1,* delims==" %%a in ("%DECOMP_PATH%\.env") do (
        set line=%%a
        REM Skip empty lines and comments
        if not "!line!"=="" (
            if not "!line:~0,1!"=="#" (
                set %%a=%%b
            )
        )
    )
    echo.
)

if not defined HAS_SET_UP_VENV set HAS_SET_UP_VENV=false

if /i "%HAS_SET_UP_VENV%"=="false" (
    echo Creating venv with Python 3.12...

    py -3.12 --version >nul 2>nul
    if errorlevel 1 (
        echo.
        echo Python 3.12 not found.
        echo Please install it from python.org first.
        echo.
        pause
        exit /b
    )

    echo Creating virtual environment...
    py -3.12 -m venv venv

    call venv\Scripts\activate.bat

    echo Installing dependencies...
    python -m pip install --upgrade pip
    python -m pip install -r requirements.txt
    set HAS_SET_UP_VENV=true
    echo HAS_SET_UP_VENV=!HAS_SET_UP_VENV! >> "%DECOMP_PATH%\.env"
)

echo Setup complete
echo Now running run.bat

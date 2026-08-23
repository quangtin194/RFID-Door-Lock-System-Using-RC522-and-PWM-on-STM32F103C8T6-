@echo off
setlocal

echo ===============================================
echo STEP 1: CLEANING BUILD DIRECTORY
echo ===============================================

REM Delete old build folder if it exists
if exist build (
    echo Deleting existing build folder...
    rmdir /s /q build
)

echo.
echo ===============================================
echo STEP 2: CONFIGURING PROJECT WITH CMAKE
echo ===============================================

REM Configure project using CMake and Ninja
cmake -B build -G Ninja --toolchain cmake/gcc-arm-none-eabi.cmake

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo ===============================================
echo STEP 3: COMPILING FIRMWARE WITH NINJA
echo ===============================================

REM Build project
cmake --build build

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ===============================================
echo STEP 4: FLASHING FIRMWARE TO TARGET MCU
echo ===============================================

REM Flash firmware to STM32 and reset MCU
STM32_Programmer_CLI -c port=SWD  -w build/PRJ.elf 0x08000000 -v -rst

if %ERRORLEVEL% neq 0 (
    echo.
    echo Flashing failed!
    pause
    exit /b 1
)

echo.
echo ===============================================
echo BUILD AND FLASH COMPLETED SUCCESSFULLY!
echo ===============================================

pause
endlocal
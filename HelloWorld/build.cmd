@echo off
REM Updating source code from GIT repository...
git pull

if not exist build (
    mkdir build
)

cd build

REM Running CMake to configure the project
cmake ..

REM Building the project
cmake --build . --config Release

REM Returning to the root directory of the project
cd ..

echo Build completed.
pause
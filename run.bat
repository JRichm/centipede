@echo off
g++ src/main.cpp -Iinclude -Llib -lSDL3 -lSDL3_image -lSDL3_mixer -o build/centipede.exe

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
build\centipede.exe
pause
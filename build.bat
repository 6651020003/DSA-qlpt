@echo off
rem Build QuanLyPhongTro.exe bang MinGW-w64 (g++). Chay file nay tai thu muc goc cua du an.
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo [LOI] Khong tim thay g++. Hay cai MinGW-w64 va them thu muc bin vao PATH.
    exit /b 1
)

if not exist build mkdir build

echo [1/2] Bien dich tai nguyen...
windres res/app.rc -O coff -o build/app.res.o
if errorlevel 1 exit /b 1

echo [2/2] Bien dich ma nguon...
g++ -std=c++17 -O2 -Wall -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 -municode -mwindows -static ^
    src/main.cpp ^
    src/core/Models.cpp src/core/RoomList.cpp src/core/TenantHash.cpp src/core/BillQueue.cpp ^
    src/services/AppData.cpp src/services/BillingService.cpp src/services/Sorter.cpp src/services/FileStorage.cpp ^
    src/gui/Controls.cpp src/gui/WinApp.cpp ^
    build/app.res.o ^
    -o QuanLyPhongTro.exe -lcomctl32 -lcomdlg32 -lgdi32 -luser32
if errorlevel 1 exit /b 1

echo Hoan tat: QuanLyPhongTro.exe
endlocal

@echo off
rem Build va chay kiem thu console cho phan core/services (khong can GUI)
setlocal
cd /d "%~dp0.."
if not exist build mkdir build
g++ -std=c++17 -Wall -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 ^
    tests/test_core.cpp ^
    src/core/Models.cpp src/core/RoomList.cpp src/core/TenantHash.cpp src/core/BillQueue.cpp ^
    src/services/AppData.cpp src/services/BillingService.cpp src/services/Sorter.cpp src/services/FileStorage.cpp ^
    -o build/test_core.exe
if errorlevel 1 exit /b 1
build\test_core.exe data
endlocal

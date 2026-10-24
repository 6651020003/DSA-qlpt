// main.cpp - [TV1] Điểm vào chương trình: khởi tạo Common Controls, nạp dữ liệu, chạy vòng lặp Windows Message
//
// Nhóm 15 - Đồ án Cấu trúc dữ liệu & Giải thuật
// Đề tài: Hệ thống Quản lý Phòng trọ & Hóa đơn Dịch vụ (Windows GUI App)

#include <windows.h>
#include <commctrl.h>
#include "services/AppData.h"
#include "services/FileStorage.h"
#include "gui/WinApp.h"
#include "gui/Controls.h"

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#endif

// Thư mục data nằm cùng cấp với file .exe; nếu không có thì dùng thư mục hiện hành
static std::wstring findDataDir() {
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exe(path, n);
    std::wstring dir = exe.substr(0, exe.find_last_of(L"\\/"));
    std::wstring data = dir + L"\\data";
    if (GetFileAttributesW(data.c_str()) != INVALID_FILE_ATTRIBUTES) return data;

    wchar_t cwd[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, cwd);
    std::wstring alt = std::wstring(cwd) + L"\\data";
    if (GetFileAttributesW(alt.c_str()) != INVALID_FILE_ATTRIBUTES) return alt;
    return data;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    AppData db;
    FileStorage storage(findDataDir());
    std::string err;
    if (!storage.loadAll(db, err)) ui::warn(NULL, ui::W(err));

    WinApp app(hInst, db, storage);
    return app.run(nCmdShow);
}

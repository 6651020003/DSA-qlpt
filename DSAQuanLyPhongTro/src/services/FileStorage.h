// FileStorage.h - [TV1] Đọc / ghi dữ liệu bền vững dạng CSV (UTF-8)
#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <string>
#include "../core/DynArray.h"

class AppData;

class FileStorage {
public:
    explicit FileStorage(const std::wstring& dataDir);

    bool loadAll(AppData& db, std::string& err);
    bool saveAll(const AppData& db, std::string& err);

    // Import danh sách phòng từ file CSV cùng định dạng PhongTro.csv; trả về số phòng thêm được
    int importRooms(const std::wstring& path, AppData& db, int& skipped, std::string& err);

    const std::wstring& dataDir() const { return m_dir; }

    // Tiện ích CSV (có hỗ trợ trường đặt trong dấu nháy kép)
    static void splitCsvLine(const std::string& line, DynArray<std::string>& out);
    static std::string csvField(const std::string& s);

private:
    std::wstring m_dir;

    std::wstring pathOf(const wchar_t* file) const;
    static bool readLines(const std::wstring& path, DynArray<std::string>& lines, bool& exists);
    static bool writeFile(const std::wstring& path, const std::string& content);
};

#endif

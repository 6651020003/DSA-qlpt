#include "FileStorage.h"
#include "AppData.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>

namespace {

const wchar_t* F_PHONG = L"PhongTro.csv";
const wchar_t* F_TAIKHOAN = L"TaiKhoan.csv";
const wchar_t* F_QUANLY = L"QuanLy.csv";
const wchar_t* F_KHACHHANG = L"KhachHang.csv";
const wchar_t* F_PHIEUTTP = L"PhieuThueTraPhong.csv";
const wchar_t* F_PHIEUTHU = L"PhieuThuTien.csv";

const char* H_PHONG = "MaPhong,GiaThue,DienTich,VatChat,TrangThai,Hinh";
const char* H_TAIKHOAN = "MaTaiKhoan,UserName,MatKhau,Role";
const char* H_QUANLY = "MaQuanLy,TenQuanLy,MaTaiKhoan";
const char* H_KHACHHANG = "MaKhachHang,TenKhachHang,DiaChi,CCCD,SDT,MaTaiKhoan";
const char* H_PHIEUTTP = "MaPhieuTTP,TenKhachHang,CCCD,SDT,DiaChi,NgayThue,NgayTra,MaQuanLy,MaPhong";
const char* H_PHIEUTHU =
    "MaPhieuThuTien,Thang,Nam,TienPhong,SoDien,SoNuoc,TienDien,TienNuoc,TienWifi,TongTien,"
    "HanThanhToan,DaThanhToan,MaQuanLy,MaKhachHang,MaPhieuTTP";

bool toNum(const std::string& s, long long& out) {
    std::string t = trimStr(s);
    if (t.empty()) return false;
    char* end = nullptr;
    out = std::strtoll(t.c_str(), &end, 10);
    return end && *end == '\0';
}

int toInt(const std::string& s, int def = 0) {
    long long v;
    return toNum(s, v) ? (int)v : def;
}

int parseTrangThai(const std::string& s) {
    std::string t = trimStr(s);
    if (t == "Đã thuê" || t == "Da thue" || t == "DaThue" || t == "1") return PHONG_DA_THUE;
    return PHONG_TRONG;
}

int parseRole(const std::string& s) {
    std::string t = trimStr(s);
    if (t == "QuanLy" || t == "Quản lý" || t == "0") return ROLE_QUAN_LY;
    return ROLE_KHACH_HANG;
}

bool parseRoom(const DynArray<std::string>& f, PhongTro& p) {
    if (f.size() < 5) return false;
    long long v;
    if (!toNum(f[0], v)) return false;
    p.maPhong = (int)v;
    if (!toNum(f[1], v)) return false;
    p.giaThue = v;
    if (!toNum(f[2], v)) return false;
    p.dienTich = (int)v;
    p.vatChat = trimStr(f[3]);
    p.trangThai = parseTrangThai(f[4]);
    p.hinh = f.size() > 5 ? trimStr(f[5]) : "";
    return true;
}

std::string num(long long v) { return std::to_string(v); }

}  // namespace

FileStorage::FileStorage(const std::wstring& dataDir) : m_dir(dataDir) {}

std::wstring FileStorage::pathOf(const wchar_t* file) const {
    return m_dir + L"\\" + file;
}

// ---------------- CSV ----------------

void FileStorage::splitCsvLine(const std::string& line, DynArray<std::string>& out) {
    out.clear();
    std::string cur;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                cur += c;
            }
        } else if (c == '"') {
            quoted = true;
        } else if (c == ',') {
            out.push(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push(cur);
}

std::string FileStorage::csvField(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
    std::string r = "\"";
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '"') r += "\"\"";
        else if (s[i] != '\n' && s[i] != '\r') r += s[i];
    }
    return r + "\"";
}

bool FileStorage::readLines(const std::wstring& path, DynArray<std::string>& lines, bool& exists) {
    lines.clear();
    FILE* f = _wfopen(path.c_str(), L"rb");
    exists = f != nullptr;
    if (!f) return false;
    std::string content;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) content.append(buf, n);
    std::fclose(f);

    size_t start = 0;
    if (content.size() >= 3 && (unsigned char)content[0] == 0xEF && (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF)
        start = 3;  // bỏ BOM UTF-8
    while (start <= content.size()) {
        size_t end = content.find('\n', start);
        if (end == std::string::npos) end = content.size();
        std::string line = content.substr(start, end - start);
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        if (!trimStr(line).empty()) lines.push(line);
        start = end + 1;
    }
    return true;
}

bool FileStorage::writeFile(const std::wstring& path, const std::string& content) {
    // Ghi ra file tạm rồi thay thế để tránh hỏng dữ liệu nếu bị ngắt giữa chừng
    std::wstring tmp = path + L".tmp";
    FILE* f = _wfopen(tmp.c_str(), L"wb");
    if (!f) return false;
    static const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};  // để Excel nhận đúng UTF-8
    bool ok = std::fwrite(bom, 1, 3, f) == 3 &&
              std::fwrite(content.data(), 1, content.size(), f) == content.size();
    ok = (std::fclose(f) == 0) && ok;
    if (!ok) {
        _wremove(tmp.c_str());
        return false;
    }
    return MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

// ---------------- Đọc ----------------

bool FileStorage::loadAll(AppData& db, std::string& err) {
    db.clear();
    CreateDirectoryW(m_dir.c_str(), NULL);
    DynArray<std::string> lines, f;
    bool exists;
    int bad = 0;

    readLines(pathOf(F_PHONG), lines, exists);
    for (int i = 1; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        PhongTro p;
        if (!parseRoom(f, p) || !db.rooms.pushBack(p)) ++bad;
    }

    readLines(pathOf(F_TAIKHOAN), lines, exists);
    for (int i = 1; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        if (f.size() < 4) { ++bad; continue; }
        TaiKhoan tk;
        tk.maTaiKhoan = toInt(f[0]);
        tk.userName = trimStr(f[1]);
        tk.matKhau = f[2];
        tk.role = parseRole(f[3]);
        if (tk.maTaiKhoan <= 0 || !db.tenants.addAccount(tk)) ++bad;
    }

    readLines(pathOf(F_QUANLY), lines, exists);
    for (int i = 1; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        if (f.size() < 2) { ++bad; continue; }
        QuanLy ql;
        ql.maQuanLy = toInt(f[0]);
        ql.tenQuanLy = trimStr(f[1]);
        ql.maTaiKhoan = f.size() > 2 ? toInt(f[2]) : 0;
        if (ql.maQuanLy > 0) db.managers.push(ql);
        else ++bad;
    }

    readLines(pathOf(F_KHACHHANG), lines, exists);
    for (int i = 1; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        if (f.size() < 5) { ++bad; continue; }
        KhachHang kh;
        kh.maKhachHang = toInt(f[0]);
        kh.tenKhachHang = trimStr(f[1]);
        kh.diaChi = trimStr(f[2]);
        kh.cccd = trimStr(f[3]);
        kh.sdt = trimStr(f[4]);
        kh.maTaiKhoan = f.size() > 5 ? toInt(f[5]) : 0;
        if (kh.maKhachHang <= 0 || !db.tenants.addCustomer(kh)) ++bad;
    }

    readLines(pathOf(F_PHIEUTTP), lines, exists);
    for (int i = 1; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        if (f.size() < 9) { ++bad; continue; }
        PhieuThueTraPhong* p = new PhieuThueTraPhong();
        p->maPhieuTTP = toInt(f[0]);
        p->tenKhachHang = trimStr(f[1]);
        p->cccd = trimStr(f[2]);
        p->sdt = trimStr(f[3]);
        p->diaChi = trimStr(f[4]);
        Date::parse(f[5], p->ngayThue);
        if (!trimStr(f[6]).empty()) Date::parse(f[6], p->ngayTra);
        p->maQuanLy = toInt(f[7]);
        p->maPhong = toInt(f[8]);
        if (p->maPhieuTTP <= 0 || p->ngayThue.empty()) {
            delete p;
            ++bad;
            continue;
        }
        db.loadRental(p);
    }

    readLines(pathOf(F_PHIEUTHU), lines, exists);
    for (int i = 1; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        if (f.size() < 15) { ++bad; continue; }
        PhieuThuTien* b = new PhieuThuTien();
        long long v;
        b->maPhieuThuTien = toInt(f[0]);
        b->thang = toInt(f[1]);
        b->nam = toInt(f[2]);
        b->tienPhong = toNum(f[3], v) ? v : 0;
        b->soDien = toInt(f[4]);
        b->soNuoc = toInt(f[5]);
        b->tienDien = toNum(f[6], v) ? v : 0;
        b->tienNuoc = toNum(f[7], v) ? v : 0;
        b->tienWifi = toNum(f[8], v) ? v : 0;
        b->tongTien = toNum(f[9], v) ? v : 0;
        Date::parse(f[10], b->hanThanhToan);
        b->daThanhToan = toInt(f[11]) != 0;
        b->maQuanLy = toInt(f[12]);
        b->maKhachHang = toInt(f[13]);
        b->maPhieuTTP = toInt(f[14]);
        if (b->maPhieuThuTien <= 0) {
            delete b;
            ++bad;
            continue;
        }
        db.loadBill(b);
    }

    db.finishLoading();
    if (bad > 0) {
        err = "Có " + std::to_string(bad) + " dòng dữ liệu không hợp lệ hoặc bị trùng đã được bỏ qua.";
        return false;
    }
    return true;
}

// ---------------- Ghi ----------------

bool FileStorage::saveAll(const AppData& db, std::string& err) {
    CreateDirectoryW(m_dir.c_str(), NULL);
    std::string s;
    bool ok = true;

    s = std::string(H_PHONG) + "\r\n";
    for (RoomNode* n = db.rooms.head(); n; n = n->next) {
        const PhongTro& p = n->data;
        s += num(p.maPhong) + "," + num(p.giaThue) + "," + num(p.dienTich) + "," + csvField(p.vatChat) + "," +
             tenTrangThaiPhong(p.trangThai) + "," + csvField(p.hinh) + "\r\n";
    }
    ok = writeFile(pathOf(F_PHONG), s) && ok;

    DynArray<TaiKhoan*> accounts;
    db.tenants.getAccounts(accounts);
    for (int i = 1; i < accounts.size(); ++i) {  // sắp chèn theo mã cho file dễ đọc
        TaiKhoan* t = accounts[i];
        int j = i - 1;
        while (j >= 0 && accounts[j]->maTaiKhoan > t->maTaiKhoan) {
            accounts[j + 1] = accounts[j];
            --j;
        }
        accounts[j + 1] = t;
    }
    s = std::string(H_TAIKHOAN) + "\r\n";
    for (int i = 0; i < accounts.size(); ++i) {
        const TaiKhoan* t = accounts[i];
        s += num(t->maTaiKhoan) + "," + csvField(t->userName) + "," + csvField(t->matKhau) + "," +
             (t->role == ROLE_QUAN_LY ? "QuanLy" : "KhachHang") + "\r\n";
    }
    ok = writeFile(pathOf(F_TAIKHOAN), s) && ok;

    s = std::string(H_QUANLY) + "\r\n";
    for (int i = 0; i < db.managers.size(); ++i) {
        const QuanLy& q = db.managers[i];
        s += num(q.maQuanLy) + "," + csvField(q.tenQuanLy) + "," + num(q.maTaiKhoan) + "\r\n";
    }
    ok = writeFile(pathOf(F_QUANLY), s) && ok;

    DynArray<KhachHang*> customers;
    db.tenants.getCustomers(customers);
    for (int i = 1; i < customers.size(); ++i) {
        KhachHang* k = customers[i];
        int j = i - 1;
        while (j >= 0 && customers[j]->maKhachHang > k->maKhachHang) {
            customers[j + 1] = customers[j];
            --j;
        }
        customers[j + 1] = k;
    }
    s = std::string(H_KHACHHANG) + "\r\n";
    for (int i = 0; i < customers.size(); ++i) {
        const KhachHang* k = customers[i];
        s += num(k->maKhachHang) + "," + csvField(k->tenKhachHang) + "," + csvField(k->diaChi) + "," + k->cccd +
             "," + k->sdt + "," + num(k->maTaiKhoan) + "\r\n";
    }
    ok = writeFile(pathOf(F_KHACHHANG), s) && ok;

    s = std::string(H_PHIEUTTP) + "\r\n";
    for (int i = 0; i < db.rentals.size(); ++i) {
        const PhieuThueTraPhong* p = db.rentals[i];
        s += num(p->maPhieuTTP) + "," + csvField(p->tenKhachHang) + "," + p->cccd + "," + p->sdt + "," +
             csvField(p->diaChi) + "," + p->ngayThue.toString() + "," + p->ngayTra.toString() + "," +
             num(p->maQuanLy) + "," + num(p->maPhong) + "\r\n";
    }
    ok = writeFile(pathOf(F_PHIEUTTP), s) && ok;

    s = std::string(H_PHIEUTHU) + "\r\n";
    for (int i = 0; i < db.bills.size(); ++i) {
        const PhieuThuTien* b = db.bills[i];
        s += num(b->maPhieuThuTien) + "," + num(b->thang) + "," + num(b->nam) + "," + num(b->tienPhong) + "," +
             num(b->soDien) + "," + num(b->soNuoc) + "," + num(b->tienDien) + "," + num(b->tienNuoc) + "," +
             num(b->tienWifi) + "," + num(b->tongTien) + "," + b->hanThanhToan.toString() + "," +
             (b->daThanhToan ? "1" : "0") + "," + num(b->maQuanLy) + "," + num(b->maKhachHang) + "," +
             num(b->maPhieuTTP) + "\r\n";
    }
    ok = writeFile(pathOf(F_PHIEUTHU), s) && ok;

    if (!ok) err = "Không ghi được dữ liệu vào thư mục data. Kiểm tra quyền ghi hoặc file đang mở trong Excel.";
    return ok;
}

// ---------------- Import ----------------

int FileStorage::importRooms(const std::wstring& path, AppData& db, int& skipped, std::string& err) {
    skipped = 0;
    DynArray<std::string> lines, f;
    bool exists;
    if (!readLines(path, lines, exists)) {
        err = "Không mở được file.";
        return -1;
    }
    int added = 0;
    for (int i = 0; i < lines.size(); ++i) {
        splitCsvLine(lines[i], f);
        PhongTro p;
        if (!parseRoom(f, p)) {
            if (i > 0) ++skipped;  // dòng đầu có thể là tiêu đề
            continue;
        }
        p.trangThai = PHONG_TRONG;  // phòng mới import chưa có phiếu thuê
        if (db.addRoom(p).empty()) ++added;
        else ++skipped;
    }
    return added;
}

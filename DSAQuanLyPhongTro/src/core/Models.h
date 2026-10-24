// Models.h - Các thực thể dữ liệu & hằng số nghiệp vụ
// Tham chiếu: Sơ đồ logic (Hình 17), Bảng tham số (Bảng 46), Quy định QD01-QD05
#ifndef MODELS_H
#define MODELS_H

#include <string>

// ===== Bảng tham số =====
const long long DON_GIA_DIEN = 2612;   // TS1: đơn giá điện (đ/kWh)
const long long DON_GIA_NUOC = 13000;  // TS2: đơn giá nước (đ/m3)
const int MAX_SDT     = 15;            // TS3: độ dài số điện thoại tối đa
const int MAX_TEN     = 30;            // TS4: độ dài tên tối đa
const int MAX_DIACHI  = 50;            // TS5: độ dài địa chỉ tối đa
const int MAX_VATCHAT = 30;            // NVARCHAR(30) trong bảng PhongTro
const int TY_LE_COC   = 50;            // QD04: tiền cọc = 50% giá thuê 1 tháng

// ===== Ngày tháng (YYYY-MM-DD) =====
struct Date {
    int y, m, d;

    Date() : y(0), m(0), d(0) {}
    Date(int yy, int mm, int dd) : y(yy), m(mm), d(dd) {}

    bool empty() const { return y == 0; }
    long toDays() const;                 // số ngày tính từ 1970-01-01
    static Date fromDays(long days);
    Date addDays(int n) const { return fromDays(toDays() + n); }
    int compare(const Date& o) const;    // <0, 0, >0
    std::string toString() const;        // "YYYY-MM-DD", rỗng nếu chưa có ngày
    std::string toDisplay() const;       // "dd/mm/yyyy"

    static bool isValid(int y, int m, int d);
    static bool parse(const std::string& s, Date& out);  // nhận YYYY-MM-DD hoặc dd/mm/yyyy
    static Date today();
};

// ===== Thực thể =====
enum TrangThaiPhong { PHONG_TRONG = 0, PHONG_DA_THUE = 1 };
enum VaiTro { ROLE_QUAN_LY = 0, ROLE_KHACH_HANG = 1 };

struct PhongTro {
    int maPhong;
    long long giaThue;
    int dienTich;
    std::string vatChat;
    int trangThai;
    std::string hinh;

    PhongTro() : maPhong(0), giaThue(0), dienTich(0), trangThai(PHONG_TRONG) {}
};

struct TaiKhoan {
    int maTaiKhoan;
    std::string userName;
    std::string matKhau;
    int role;

    TaiKhoan() : maTaiKhoan(0), role(ROLE_KHACH_HANG) {}
};

// Bổ sung MaTaiKhoan để liên kết tài khoản đăng nhập với người quản lý
struct QuanLy {
    int maQuanLy;
    std::string tenQuanLy;
    int maTaiKhoan;

    QuanLy() : maQuanLy(0), maTaiKhoan(0) {}
};

struct KhachHang {
    int maKhachHang;
    std::string tenKhachHang;
    std::string diaChi;
    std::string cccd;
    std::string sdt;
    int maTaiKhoan;  // 0 = chưa có tài khoản

    KhachHang() : maKhachHang(0), maTaiKhoan(0) {}
};

struct PhieuThueTraPhong {
    int maPhieuTTP;
    std::string tenKhachHang;
    std::string cccd;
    std::string sdt;
    std::string diaChi;
    Date ngayThue;
    Date ngayTra;  // rỗng = đang thuê
    int maQuanLy;
    int maPhong;

    PhieuThueTraPhong() : maPhieuTTP(0), maQuanLy(0), maPhong(0) {}
    bool dangThue() const { return ngayTra.empty(); }
};

struct PhieuThuTien {
    int maPhieuThuTien;
    int thang, nam;          // kỳ thu tiền
    long long tienPhong;
    int soDien;              // kWh
    int soNuoc;              // m3
    long long tienDien;
    long long tienNuoc;
    long long tienWifi;
    long long tongTien;
    Date hanThanhToan;
    bool daThanhToan;
    int maQuanLy;
    int maKhachHang;
    int maPhieuTTP;

    int maPhong;    // lấy từ phiếu thuê, không lưu file
    int heapIndex;  // vị trí trong BillQueue (-1 = không nằm trong hàng đợi)

    PhieuThuTien()
        : maPhieuThuTien(0), thang(0), nam(0), tienPhong(0), soDien(0), soNuoc(0),
          tienDien(0), tienNuoc(0), tienWifi(0), tongTien(0), daThanhToan(false),
          maQuanLy(0), maKhachHang(0), maPhieuTTP(0), maPhong(0), heapIndex(-1) {}
};

// ===== Tiện ích =====
const char* tenTrangThaiPhong(int trangThai);
const char* tenVaiTro(int role);
int utf8Length(const std::string& s);         // số ký tự (không phải số byte)
bool isDigits(const std::string& s);
std::string trimStr(const std::string& s);

#endif

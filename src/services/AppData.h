// AppData.h - Kho dữ liệu trong bộ nhớ + các nghiệp vụ (thuê/trả phòng, phiếu thu, tài khoản)
// Mỗi hàm nghiệp vụ trả về chuỗi rỗng nếu thành công, ngược lại là thông báo lỗi (UTF-8).
#ifndef APP_DATA_H
#define APP_DATA_H

#include <string>
#include "../core/Models.h"
#include "../core/DynArray.h"
#include "../core/RoomList.h"
#include "../core/TenantHash.h"
#include "../core/BillQueue.h"

class AppData {
public:
    RoomList rooms;                         // DSLK đôi
    TenantHash tenants;                     // Bảng băm khách hàng + tài khoản
    DynArray<QuanLy> managers;
    DynArray<PhieuThueTraPhong*> rentals;   // sở hữu con trỏ
    DynArray<PhieuThuTien*> bills;          // sở hữu con trỏ
    BillQueue unpaid;                       // Min-Heap các phiếu chưa thanh toán

    AppData() {}
    ~AppData();
    AppData(const AppData&) = delete;
    AppData& operator=(const AppData&) = delete;
    void clear();

    // ----- Sinh mã mới -----
    int nextMaKhachHang() const;
    int nextMaTaiKhoan() const;
    int nextMaQuanLy() const;
    int nextMaPhieuTTP() const;
    int nextMaPhieuThu() const;

    // ----- Tra cứu -----
    PhieuThueTraPhong* findRental(int maPhieuTTP) const;
    PhieuThueTraPhong* activeRentalOfRoom(int maPhong) const;
    PhieuThueTraPhong* activeRentalOfCustomer(const std::string& cccd) const;
    PhieuThuTien* findBill(int maPhieu) const;
    QuanLy* findManager(int maQuanLy) const;
    QuanLy* findManagerByAccount(int maTaiKhoan) const;
    bool billExists(int maPhieuTTP, int thang, int nam) const;
    int unpaidCountOfRental(int maPhieuTTP, long long* tongNo = nullptr) const;

    // ----- Phòng trọ (QD01) -----
    std::string addRoom(const PhongTro& p);
    std::string updateRoom(const PhongTro& p);
    std::string deleteRoom(int maPhong);

    // ----- Khách hàng (QD02) -----
    static std::string validateCustomer(const KhachHang& kh);
    std::string addCustomer(KhachHang& kh);  // gán mã mới vào kh
    std::string updateCustomer(const KhachHang& kh);
    std::string deleteCustomer(int maKhachHang);

    // ----- Tài khoản -----
    TaiKhoan* login(const std::string& user, const std::string& pass) const;
    std::string registerCustomer(const std::string& user, const std::string& pass, KhachHang info);
    std::string addManagerAccount(const std::string& user, const std::string& pass, const std::string& tenQuanLy);
    std::string changePassword(int maTaiKhoan, const std::string& oldPass, const std::string& newPass, bool checkOld);
    std::string deleteAccount(int maTaiKhoan, int currentUser);

    // ----- Thuê / trả phòng -----
    std::string rentRoom(PhieuThueTraPhong& p, const KhachHang& info);
    std::string returnRoom(int maPhieuTTP, const Date& ngayTra);
    std::string deleteRental(int maPhieuTTP);

    // ----- Phiếu thu tiền (QD03) -----
    std::string createBill(PhieuThuTien& b);
    std::string payBill(int maPhieu);
    std::string deleteBill(int maPhieu);

    // Nạp từ file (không kiểm tra nghiệp vụ), gọi bởi FileStorage
    void loadRental(PhieuThueTraPhong* p) { rentals.push(p); }
    void loadBill(PhieuThuTien* b);
    void finishLoading();  // gán MaPhong cho phiếu thu, tạo tài khoản quản trị nếu dữ liệu rỗng

private:
    static std::string validateAccount(const std::string& user, const std::string& pass);
};

#endif

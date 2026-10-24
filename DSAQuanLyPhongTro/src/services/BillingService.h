// BillingService.h - [TV5] Logic tính hóa đơn điện/nước & các hàm thống kê
#ifndef BILLING_SERVICE_H
#define BILLING_SERVICE_H

#include "../core/Models.h"
#include "../core/DynArray.h"

class AppData;

struct RevenueRow {
    int thang;
    int soPhieu;
    long long tienPhong, tienDien, tienNuoc, tienWifi, tong;
    RevenueRow() : thang(0), soPhieu(0), tienPhong(0), tienDien(0), tienNuoc(0), tienWifi(0), tong(0) {}
};

struct LoyalCustomer {
    KhachHang* kh;
    int soLanThue;
    int soThangThue;
    long long tongDaThanhToan;
    LoyalCustomer() : kh(nullptr), soLanThue(0), soThangThue(0), tongDaThanhToan(0) {}
};

enum LoyalCriteria { LOYAL_BY_MONTHS = 0, LOYAL_BY_TIMES = 1, LOYAL_BY_PAID = 2 };

class BillingService {
public:
    // ----- Tính tiền -----
    static long long tienDien(int soKwh) { return (long long)soKwh * DON_GIA_DIEN; }
    static long long tienNuoc(int soKhoi) { return (long long)soKhoi * DON_GIA_NUOC; }
    static long long tienCoc(long long giaThue) { return giaThue * TY_LE_COC / 100; }
    static void tinhHoaDon(PhieuThuTien& b);  // điền TienDien, TienNuoc, TongTien

    static int soNgayQuaHan(const PhieuThuTien& b, const Date& today);
    static const char* mucDoKhanCap(int soNgayQuaHan);
    static int soThangThue(const PhieuThueTraPhong& p, const Date& today);

    // ----- Thống kê -----
    // Doanh thu (phiếu đã thanh toán) 12 tháng của một năm; trả về tổng cả năm
    static long long doanhThuNam(const AppData& db, int nam, RevenueRow rows[12]);
    // Danh sách phiếu đã thanh toán của 1 tháng; trả về tổng doanh thu tháng
    static long long doanhThuThang(const AppData& db, int thang, int nam, DynArray<PhieuThuTien*>& out);
    // Phiếu chưa thanh toán theo thứ tự Min-Heap (thang = 0: mọi tháng); trả về tổng nợ
    static long long chuaThanhToan(const AppData& db, int thang, int nam, DynArray<PhieuThuTien*>& out);
    // Khách hàng thân thiết: gom theo khách (bảng băm) rồi QuickSort giảm dần theo tiêu chí
    static void khachHangThanThiet(const AppData& db, LoyalCriteria tieuChi, long long toiThieu,
                                   DynArray<LoyalCustomer>& out);
    static long long giaTriTieuChi(const LoyalCustomer& c, LoyalCriteria tieuChi);
};

#endif

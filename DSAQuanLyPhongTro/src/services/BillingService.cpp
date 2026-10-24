#include "BillingService.h"
#include "AppData.h"
#include "Sorter.h"

void BillingService::tinhHoaDon(PhieuThuTien& b) {
    b.tienDien = tienDien(b.soDien);
    b.tienNuoc = tienNuoc(b.soNuoc);
    b.tongTien = b.tienPhong + b.tienDien + b.tienNuoc + b.tienWifi;
}

int BillingService::soNgayQuaHan(const PhieuThuTien& b, const Date& today) {
    if (b.daThanhToan || b.hanThanhToan.empty()) return 0;
    return (int)(today.toDays() - b.hanThanhToan.toDays());
}

const char* BillingService::mucDoKhanCap(int soNgayQuaHan) {
    if (soNgayQuaHan > 30) return "Rất khẩn cấp";
    if (soNgayQuaHan > 0) return "Khẩn cấp";
    if (soNgayQuaHan > -5) return "Sắp đến hạn";
    return "Bình thường";
}

int BillingService::soThangThue(const PhieuThueTraPhong& p, const Date& today) {
    Date end = p.ngayTra.empty() ? today : p.ngayTra;
    long days = end.toDays() - p.ngayThue.toDays();
    if (days < 0) return 0;
    int months = (int)((days + 29) / 30);
    return months < 1 ? 1 : months;
}

long long BillingService::doanhThuNam(const AppData& db, int nam, RevenueRow rows[12]) {
    for (int i = 0; i < 12; ++i) {
        rows[i] = RevenueRow();
        rows[i].thang = i + 1;
    }
    long long tong = 0;
    for (int i = 0; i < db.bills.size(); ++i) {
        const PhieuThuTien* b = db.bills[i];
        if (!b->daThanhToan || b->nam != nam || b->thang < 1 || b->thang > 12) continue;
        RevenueRow& r = rows[b->thang - 1];
        r.soPhieu++;
        r.tienPhong += b->tienPhong;
        r.tienDien += b->tienDien;
        r.tienNuoc += b->tienNuoc;
        r.tienWifi += b->tienWifi;
        r.tong += b->tongTien;
        tong += b->tongTien;
    }
    return tong;
}

long long BillingService::doanhThuThang(const AppData& db, int thang, int nam, DynArray<PhieuThuTien*>& out) {
    out.clear();
    long long tong = 0;
    for (int i = 0; i < db.bills.size(); ++i) {
        PhieuThuTien* b = db.bills[i];
        if (b->daThanhToan && b->thang == thang && b->nam == nam) {
            out.push(b);
            tong += b->tongTien;
        }
    }
    Sorter::quickSortBills(out, BILL_BY_MA, true);
    return tong;
}

long long BillingService::chuaThanhToan(const AppData& db, int thang, int nam, DynArray<PhieuThuTien*>& out) {
    DynArray<PhieuThuTien*> all;
    db.unpaid.toSortedArray(all);
    out.clear();
    long long tong = 0;
    for (int i = 0; i < all.size(); ++i) {
        PhieuThuTien* b = all[i];
        if (thang > 0 && (b->thang != thang || b->nam != nam)) continue;
        out.push(b);
        tong += b->tongTien;
    }
    return tong;
}

long long BillingService::giaTriTieuChi(const LoyalCustomer& c, LoyalCriteria tieuChi) {
    switch (tieuChi) {
    case LOYAL_BY_TIMES: return c.soLanThue;
    case LOYAL_BY_PAID: return c.tongDaThanhToan;
    default: return c.soThangThue;
    }
}

void BillingService::khachHangThanThiet(const AppData& db, LoyalCriteria tieuChi, long long toiThieu,
                                        DynArray<LoyalCustomer>& out) {
    Date today = Date::today();
    DynArray<LoyalCustomer> all;
    HashTable<int, int> viTri;  // Mã KH -> vị trí trong mảng all

    for (int i = 0; i < db.rentals.size(); ++i) {
        const PhieuThueTraPhong* p = db.rentals[i];
        KhachHang* kh = db.tenants.findByCCCD(p->cccd);
        if (!kh) continue;
        int* idx = viTri.find(kh->maKhachHang);
        int k;
        if (idx) {
            k = *idx;
        } else {
            LoyalCustomer c;
            c.kh = kh;
            all.push(c);
            k = all.size() - 1;
            viTri.insert(kh->maKhachHang, k);
        }
        all[k].soLanThue++;
        all[k].soThangThue += soThangThue(*p, today);
    }

    for (int i = 0; i < db.bills.size(); ++i) {
        const PhieuThuTien* b = db.bills[i];
        if (!b->daThanhToan) continue;
        int* idx = viTri.find(b->maKhachHang);
        if (idx) all[*idx].tongDaThanhToan += b->tongTien;
    }

    out.clear();
    for (int i = 0; i < all.size(); ++i)
        if (giaTriTieuChi(all[i], tieuChi) >= toiThieu) out.push(all[i]);

    Sorter::quickSort(out, [tieuChi](const LoyalCustomer& a, const LoyalCustomer& b) {
        long long ka = giaTriTieuChi(a, tieuChi), kb = giaTriTieuChi(b, tieuChi);
        if (ka != kb) return ka > kb;
        if (a.soThangThue != b.soThangThue) return a.soThangThue > b.soThangThue;
        return a.kh->maKhachHang < b.kh->maKhachHang;
    });
}

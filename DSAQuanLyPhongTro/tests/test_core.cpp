// test_core.cpp - Kiểm thử nhanh các cấu trúc dữ liệu & nghiệp vụ (chạy trên console, không cần GUI)
// Build: xem tests/run_tests.bat. Chương trình chép dữ liệu mẫu sang thư mục tạm nên không làm hỏng data/.
#include <cstdio>
#include <string>
#include <windows.h>
#include "../src/core/RoomList.h"
#include "../src/core/TenantHash.h"
#include "../src/core/BillQueue.h"
#include "../src/services/AppData.h"
#include "../src/services/BillingService.h"
#include "../src/services/FileStorage.h"
#include "../src/services/Sorter.h"

static int g_fail = 0, g_pass = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) ++g_pass;                                               \
        else {                                                            \
            ++g_fail;                                                     \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

static PhongTro room(int ma, long long gia, int dt) {
    PhongTro p;
    p.maPhong = ma;
    p.giaThue = gia;
    p.dienTich = dt;
    p.vatChat = "test";
    return p;
}

static void testRoomList() {
    std::printf("[RoomList - DSLK doi]\n");
    RoomList l;
    CHECK(l.pushBack(room(3, 3000, 30)));
    CHECK(l.pushBack(room(1, 1000, 10)));
    CHECK(l.pushFront(room(2, 2000, 20)));
    CHECK(!l.pushBack(room(1, 1, 1)));  // trùng mã
    CHECK(l.size() == 3 && l.head()->data.maPhong == 2 && l.tail()->data.maPhong == 1);
    CHECK(l.remove(3));
    CHECK(l.head()->next == l.tail() && l.tail()->prev == l.head());
    CHECK(!l.remove(99));
    for (int i = 10; i < 60; ++i) l.pushBack(room(i, (i * 7919) % 50 * 100000 + 1000000, 10 + i % 25));
    Sorter::mergeSortRooms(l, ROOM_BY_GIA, true);
    bool sorted = true, linksOk = true;
    int count = 0;
    for (RoomNode* n = l.head(); n; n = n->next, ++count) {
        if (n->next && n->data.giaThue > n->next->data.giaThue) sorted = false;
        if (n->next && n->next->prev != n) linksOk = false;
    }
    CHECK(sorted && linksOk && count == l.size() && l.tail()->next == nullptr);
    Sorter::mergeSortRooms(l, ROOM_BY_DIEN_TICH, false);
    sorted = true;
    for (RoomNode* n = l.head(); n && n->next; n = n->next)
        if (n->data.dienTich < n->next->data.dienTich) sorted = false;
    CHECK(sorted);
}

static void testHash() {
    std::printf("[TenantHash - Bang bam]\n");
    TenantHash h;
    for (int i = 1; i <= 500; ++i) {
        KhachHang k;
        k.maKhachHang = i;
        k.tenKhachHang = "KH" + std::to_string(i);
        k.cccd = std::to_string(100000000000LL + i);
        k.sdt = std::to_string(900000000 + i);
        CHECK(h.addCustomer(k));
    }
    CHECK(h.customerCount() == 500 && h.customerBuckets() > 500);  // đã rehash
    CHECK(h.findByCCCD("100000000250")->maKhachHang == 250);
    CHECK(h.findBySDT("900000007")->maKhachHang == 7);
    KhachHang dup;
    dup.maKhachHang = 999;
    dup.cccd = "100000000001";
    dup.sdt = "1";
    CHECK(!h.addCustomer(dup));  // trùng CCCD
    KhachHang u = *h.findCustomer(10);
    u.cccd = "555555555555";
    CHECK(h.updateCustomer(u));
    CHECK(h.findByCCCD("100000000010") == nullptr && h.findByCCCD("555555555555")->maKhachHang == 10);
    CHECK(h.removeCustomer(10) && !h.findCustomer(10) && !h.findByCCCD("555555555555"));
    std::printf("  500 khach: %d bucket, chuoi dai nhat = %d\n", h.customerBuckets(), h.customerLongestChain());
}

static void testHeap() {
    std::printf("[BillQueue - Min-Heap]\n");
    BillQueue q;
    PhieuThuTien b[50];
    for (int i = 0; i < 50; ++i) {
        b[i].maPhieuThuTien = i + 1;
        b[i].hanThanhToan = Date(2026, 1, 1).addDays((i * 17) % 50);
        b[i].tongTien = (i % 3) * 1000;
        q.push(&b[i]);
    }
    CHECK(q.size() == 50);
    CHECK(q.remove(&b[0]) && q.remove(&b[25]) && !q.remove(&b[0]));
    DynArray<PhieuThuTien*> sorted;
    q.toSortedArray(sorted);
    bool ok = sorted.size() == 48;
    for (int i = 0; i + 1 < sorted.size(); ++i)
        if (BillQueue::higherPriority(sorted[i + 1], sorted[i])) ok = false;
    CHECK(ok);
    CHECK(q.size() == 48 && q.top() == sorted[0]);
    PhieuThuTien* prev = q.pop();
    ok = true;
    while (!q.empty()) {
        PhieuThuTien* t = q.pop();
        if (BillQueue::higherPriority(t, prev)) ok = false;
        prev = t;
    }
    CHECK(ok);
}

static void testSearch() {
    std::printf("[Binary Search - tim phong theo khoang gia]\n");
    RoomList l;
    for (int i = 1; i <= 100; ++i) l.pushBack(room(i, (long long)((i * 37) % 100) * 50000 + 1000000, 10 + i % 30));
    RoomFilter f;
    f.giaMin = 2000000;
    f.giaMax = 3000000;
    f.dtMin = 15;
    DynArray<PhongTro*> out;
    Sorter::searchRooms(l, f, out);
    int expect = 0;
    for (RoomNode* n = l.head(); n; n = n->next)
        if (n->data.giaThue >= 2000000 && n->data.giaThue <= 3000000 && n->data.dienTich >= 15) ++expect;
    CHECK(out.size() == expect && expect > 0);
    bool ok = true;
    for (int i = 0; i < out.size(); ++i)
        if (out[i]->giaThue < 2000000 || out[i]->giaThue > 3000000 || out[i]->dienTich < 15) ok = false;
    CHECK(ok);
}

static void testBusiness(const std::wstring& dataDir) {
    std::printf("[Nghiep vu tren du lieu mau]\n");
    AppData db;
    FileStorage fs(dataDir);
    std::string err;
    CHECK(fs.loadAll(db, err));
    if (!err.empty()) std::printf("  load: %s\n", err.c_str());
    CHECK(db.rooms.size() == 12 && db.tenants.customerCount() == 8 && db.rentals.size() == 7 && db.bills.size() == 37);
    CHECK(db.unpaid.size() == 7);
    CHECK(db.login("admin", "admin123") != nullptr && db.login("admin", "sai") == nullptr);

    // Hóa đơn: 120 kWh, 5 m3, phòng 2.500.000, wifi 100.000
    PhieuThuTien t;
    t.tienPhong = 2500000;
    t.soDien = 120;
    t.soNuoc = 5;
    t.tienWifi = 100000;
    BillingService::tinhHoaDon(t);
    CHECK(t.tienDien == 313440 && t.tienNuoc == 65000 && t.tongTien == 2978440);

    // Thuê phòng trống -> phòng chuyển Đã thuê, khách mới được tạo
    PhieuThueTraPhong p;
    p.maPhong = 103;
    p.ngayThue = Date(2026, 10, 1);
    p.maQuanLy = 1;
    KhachHang kh;
    kh.tenKhachHang = "Khách Kiểm Thử";
    kh.cccd = "079999999999";
    kh.sdt = "0999999999";
    kh.diaChi = "Địa chỉ test";
    CHECK(db.rentRoom(p, kh).empty());
    CHECK(db.rooms.find(103)->data.trangThai == PHONG_DA_THUE);
    CHECK(db.tenants.findByCCCD("079999999999") != nullptr);
    CHECK(!db.rentRoom(p, kh).empty());  // phòng đã thuê
    CHECK(!db.deleteRoom(103).empty());  // không xóa phòng đang thuê

    // Lập phiếu thu + QD03
    PhieuThuTien b;
    b.maPhieuTTP = p.maPhieuTTP;
    b.thang = 10;
    b.nam = 2026;
    b.soDien = 100;
    b.soNuoc = 4;
    b.tienWifi = 100000;
    b.hanThanhToan = Date(2026, 9, 1);  // hạn rất sớm -> phải lên đỉnh heap
    b.maQuanLy = 1;
    CHECK(db.createBill(b).empty());
    CHECK(b.tongTien == 2800000 + 261200 + 52000 + 100000);
    PhieuThuTien b2 = b;
    CHECK(!db.createBill(b2).empty());  // trùng tháng
    CHECK(db.unpaid.size() == 8 && db.unpaid.top()->maPhieuThuTien == b.maPhieuThuTien);
    CHECK(db.payBill(b.maPhieuThuTien).empty() && db.unpaid.size() == 7);
    CHECK(!db.payBill(b.maPhieuThuTien).empty());

    // Trả phòng
    CHECK(!db.returnRoom(p.maPhieuTTP, Date(2026, 9, 1)).empty());  // trước ngày thuê
    CHECK(db.returnRoom(p.maPhieuTTP, Date(2026, 10, 20)).empty());
    CHECK(db.rooms.find(103)->data.trangThai == PHONG_TRONG);

    // Thống kê
    RevenueRow rows[12];
    long long tongNam = BillingService::doanhThuNam(db, 2026, rows);
    long long sum = 0;
    for (int i = 0; i < db.bills.size(); ++i)
        if (db.bills[i]->daThanhToan && db.bills[i]->nam == 2026) sum += db.bills[i]->tongTien;
    CHECK(tongNam == sum && tongNam > 0);
    std::printf("  Doanh thu 2026: %lld\n", tongNam);

    DynArray<PhieuThuTien*> no;
    long long tongNo = BillingService::chuaThanhToan(db, 0, 0, no);
    CHECK(no.size() == 7 && tongNo > 0);
    std::printf("  Chua thanh toan: %d phieu, phieu dau heap #%d han %s\n", no.size(), no[0]->maPhieuThuTien,
                no[0]->hanThanhToan.toString().c_str());

    DynArray<LoyalCustomer> loyal;
    BillingService::khachHangThanThiet(db, LOYAL_BY_TIMES, 2, loyal);
    CHECK(loyal.size() == 1 && loyal[0].kh->maKhachHang == 3);  // Trần Quốc Bảo thuê 2 lần
    BillingService::khachHangThanThiet(db, LOYAL_BY_MONTHS, 0, loyal);
    bool desc = true;
    for (int i = 0; i + 1 < loyal.size(); ++i)
        if (loyal[i].soThangThue < loyal[i + 1].soThangThue) desc = false;
    CHECK(desc);

    // Không xóa được khách đã có phiếu
    CHECK(!db.deleteCustomer(1).empty());
    CHECK(db.deleteCustomer(8).empty());

    // Lưu rồi nạp lại phải giữ nguyên số lượng
    CHECK(fs.saveAll(db, err));
    AppData db2;
    CHECK(fs.loadAll(db2, err));
    CHECK(db2.rooms.size() == db.rooms.size() && db2.bills.size() == db.bills.size() &&
          db2.rentals.size() == db.rentals.size() && db2.tenants.customerCount() == db.tenants.customerCount() &&
          db2.unpaid.size() == db.unpaid.size());
    CHECK(db2.rooms.find(101)->data.vatChat == "1 giường, 1 quạt, 1 tủ");
}

static void copyDir(const std::wstring& from, const std::wstring& to) {
    CreateDirectoryW(to.c_str(), NULL);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((from + L"\\*.csv").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        CopyFileW((from + L"\\" + fd.cFileName).c_str(), (to + L"\\" + fd.cFileName).c_str(), FALSE);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

int main(int argc, char** argv) {
    testRoomList();
    testHash();
    testHeap();
    testSearch();

    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring work = std::wstring(tmp) + L"qlpt_test_data";
    std::wstring src = L"data";
    if (argc > 1) {
        int n = MultiByteToWideChar(CP_ACP, 0, argv[1], -1, NULL, 0);
        std::wstring w(n, 0);
        MultiByteToWideChar(CP_ACP, 0, argv[1], -1, &w[0], n);
        src = w.c_str();
    }
    copyDir(src, work);
    testBusiness(work);

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

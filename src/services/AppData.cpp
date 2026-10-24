#include "AppData.h"
#include "BillingService.h"

AppData::~AppData() { clear(); }

void AppData::clear() {
    unpaid.clear();
    for (int i = 0; i < bills.size(); ++i) delete bills[i];
    for (int i = 0; i < rentals.size(); ++i) delete rentals[i];
    bills.clear();
    rentals.clear();
    managers.clear();
    tenants.clear();
    rooms.clear();
}

// ================= Sinh mã =================

int AppData::nextMaKhachHang() const { return tenants.maxCustomerId() + 1; }
int AppData::nextMaTaiKhoan() const { return tenants.maxAccountId() + 1; }

int AppData::nextMaQuanLy() const {
    int mx = 0;
    for (int i = 0; i < managers.size(); ++i)
        if (managers[i].maQuanLy > mx) mx = managers[i].maQuanLy;
    return mx + 1;
}

int AppData::nextMaPhieuTTP() const {
    int mx = 0;
    for (int i = 0; i < rentals.size(); ++i)
        if (rentals[i]->maPhieuTTP > mx) mx = rentals[i]->maPhieuTTP;
    return mx + 1;
}

int AppData::nextMaPhieuThu() const {
    int mx = 0;
    for (int i = 0; i < bills.size(); ++i)
        if (bills[i]->maPhieuThuTien > mx) mx = bills[i]->maPhieuThuTien;
    return mx + 1;
}

// ================= Tra cứu =================

PhieuThueTraPhong* AppData::findRental(int maPhieuTTP) const {
    for (int i = 0; i < rentals.size(); ++i)
        if (rentals[i]->maPhieuTTP == maPhieuTTP) return rentals[i];
    return nullptr;
}

PhieuThueTraPhong* AppData::activeRentalOfRoom(int maPhong) const {
    for (int i = 0; i < rentals.size(); ++i)
        if (rentals[i]->maPhong == maPhong && rentals[i]->dangThue()) return rentals[i];
    return nullptr;
}

PhieuThueTraPhong* AppData::activeRentalOfCustomer(const std::string& cccd) const {
    for (int i = 0; i < rentals.size(); ++i)
        if (rentals[i]->cccd == cccd && rentals[i]->dangThue()) return rentals[i];
    return nullptr;
}

PhieuThuTien* AppData::findBill(int maPhieu) const {
    for (int i = 0; i < bills.size(); ++i)
        if (bills[i]->maPhieuThuTien == maPhieu) return bills[i];
    return nullptr;
}

QuanLy* AppData::findManager(int maQuanLy) const {
    for (int i = 0; i < managers.size(); ++i)
        if (managers[i].maQuanLy == maQuanLy) return const_cast<QuanLy*>(&managers[i]);
    return nullptr;
}

QuanLy* AppData::findManagerByAccount(int maTaiKhoan) const {
    for (int i = 0; i < managers.size(); ++i)
        if (managers[i].maTaiKhoan == maTaiKhoan) return const_cast<QuanLy*>(&managers[i]);
    return nullptr;
}

bool AppData::billExists(int maPhieuTTP, int thang, int nam) const {
    for (int i = 0; i < bills.size(); ++i) {
        const PhieuThuTien* b = bills[i];
        if (b->maPhieuTTP == maPhieuTTP && b->thang == thang && b->nam == nam) return true;
    }
    return false;
}

int AppData::unpaidCountOfRental(int maPhieuTTP, long long* tongNo) const {
    int c = 0;
    long long sum = 0;
    for (int i = 0; i < bills.size(); ++i) {
        if (bills[i]->maPhieuTTP == maPhieuTTP && !bills[i]->daThanhToan) {
            ++c;
            sum += bills[i]->tongTien;
        }
    }
    if (tongNo) *tongNo = sum;
    return c;
}

// ================= Phòng trọ =================

static std::string validateRoom(const PhongTro& p) {
    if (p.maPhong <= 0) return "Mã phòng phải là số nguyên dương.";
    if (p.giaThue <= 0) return "Giá thuê phải lớn hơn 0.";
    if (p.dienTich <= 0) return "Diện tích phải lớn hơn 0.";
    if (utf8Length(p.vatChat) > MAX_VATCHAT) return "Vật chất tối đa 30 ký tự.";
    if (p.trangThai != PHONG_TRONG && p.trangThai != PHONG_DA_THUE) return "Trạng thái phòng không hợp lệ.";
    return "";
}

std::string AppData::addRoom(const PhongTro& p) {
    std::string err = validateRoom(p);
    if (!err.empty()) return err;
    if (!rooms.pushBack(p)) return "Mã phòng " + std::to_string(p.maPhong) + " đã tồn tại.";
    return "";
}

std::string AppData::updateRoom(const PhongTro& p) {
    std::string err = validateRoom(p);
    if (!err.empty()) return err;
    if (!rooms.find(p.maPhong)) return "Không tìm thấy phòng cần sửa.";
    if (p.trangThai == PHONG_TRONG && activeRentalOfRoom(p.maPhong))
        return "Phòng đang có phiếu thuê. Hãy lập phiếu trả phòng thay vì đổi trạng thái.";
    rooms.update(p);
    return "";
}

std::string AppData::deleteRoom(int maPhong) {
    if (!rooms.find(maPhong)) return "Không tìm thấy phòng.";
    if (activeRentalOfRoom(maPhong)) return "Không thể xóa phòng đang có người thuê.";
    rooms.remove(maPhong);
    return "";
}

// ================= Khách hàng =================

std::string AppData::validateCustomer(const KhachHang& kh) {
    if (kh.tenKhachHang.empty()) return "Vui lòng nhập tên khách hàng.";
    if (utf8Length(kh.tenKhachHang) > MAX_TEN) return "Tên khách hàng tối đa 30 ký tự (TS4).";
    if (kh.diaChi.empty()) return "Vui lòng nhập địa chỉ.";
    if (utf8Length(kh.diaChi) > MAX_DIACHI) return "Địa chỉ tối đa 50 ký tự (TS5).";
    if (!isDigits(kh.cccd) || (kh.cccd.size() != 9 && kh.cccd.size() != 12))
        return "CCCD phải gồm 12 chữ số (hoặc CMND 9 chữ số).";
    if (!isDigits(kh.sdt) || kh.sdt.size() < 9 || (int)kh.sdt.size() > MAX_SDT)
        return "Số điện thoại chỉ gồm chữ số, từ 9 đến 15 số (TS3).";
    return "";
}

std::string AppData::addCustomer(KhachHang& kh) {
    std::string err = validateCustomer(kh);
    if (!err.empty()) return err;
    if (tenants.findByCCCD(kh.cccd)) return "CCCD đã tồn tại trong hệ thống.";
    if (tenants.findBySDT(kh.sdt)) return "Số điện thoại đã được khách hàng khác sử dụng.";
    kh.maKhachHang = nextMaKhachHang();
    if (!tenants.addCustomer(kh)) return "Không thể thêm khách hàng.";
    return "";
}

std::string AppData::updateCustomer(const KhachHang& kh) {
    std::string err = validateCustomer(kh);
    if (!err.empty()) return err;
    KhachHang* cur = tenants.findCustomer(kh.maKhachHang);
    if (!cur) return "Không tìm thấy khách hàng.";
    KhachHang* other = tenants.findByCCCD(kh.cccd);
    if (other && other != cur) return "CCCD đã thuộc về khách hàng khác.";
    other = tenants.findBySDT(kh.sdt);
    if (other && other != cur) return "Số điện thoại đã thuộc về khách hàng khác.";

    // Đồng bộ thông tin trên các phiếu thuê đang hiệu lực (phiếu tra cứu khách theo CCCD)
    std::string oldCccd = cur->cccd;
    for (int i = 0; i < rentals.size(); ++i) {
        PhieuThueTraPhong* p = rentals[i];
        if (p->cccd != oldCccd) continue;
        p->cccd = kh.cccd;
        if (p->dangThue()) {
            p->tenKhachHang = kh.tenKhachHang;
            p->sdt = kh.sdt;
            p->diaChi = kh.diaChi;
        }
    }
    tenants.updateCustomer(kh);
    return "";
}

std::string AppData::deleteCustomer(int maKhachHang) {
    KhachHang* kh = tenants.findCustomer(maKhachHang);
    if (!kh) return "Không tìm thấy khách hàng.";
    for (int i = 0; i < rentals.size(); ++i)
        if (rentals[i]->cccd == kh->cccd)
            return "Khách hàng đã có phiếu thuê/trả phòng nên không thể xóa.";
    for (int i = 0; i < bills.size(); ++i)
        if (bills[i]->maKhachHang == maKhachHang)
            return "Khách hàng đã có phiếu thu tiền nên không thể xóa.";
    int maTK = kh->maTaiKhoan;
    tenants.removeCustomer(maKhachHang);
    if (maTK > 0) tenants.removeAccount(maTK);
    return "";
}

// ================= Tài khoản =================

std::string AppData::validateAccount(const std::string& user, const std::string& pass) {
    if (user.size() < 3 || utf8Length(user) > 30) return "Tên đăng nhập phải từ 3 đến 30 ký tự.";
    for (size_t i = 0; i < user.size(); ++i) {
        char c = user[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.';
        if (!ok) return "Tên đăng nhập chỉ gồm chữ không dấu, số, dấu _ hoặc dấu chấm.";
    }
    if (pass.size() < 4 || pass.size() > 30) return "Mật khẩu phải từ 4 đến 30 ký tự.";
    if (pass.find(',') != std::string::npos || pass.find('"') != std::string::npos)
        return "Mật khẩu không được chứa dấu phẩy hoặc dấu nháy kép.";
    return "";
}

TaiKhoan* AppData::login(const std::string& user, const std::string& pass) const {
    TaiKhoan* tk = tenants.findAccount(user);  // tra cứu O(1) trên bảng băm
    if (tk && tk->matKhau == pass) return tk;
    return nullptr;
}

std::string AppData::registerCustomer(const std::string& user, const std::string& pass, KhachHang info) {
    std::string err = validateAccount(user, pass);
    if (!err.empty()) return err;
    if (tenants.findAccount(user)) return "Tên đăng nhập đã tồn tại.";

    KhachHang* existing = tenants.findByCCCD(info.cccd);
    if (existing && existing->maTaiKhoan > 0) return "CCCD này đã được đăng ký tài khoản.";
    if (!existing) {
        err = validateCustomer(info);
        if (!err.empty()) return err;
        if (tenants.findBySDT(info.sdt)) return "Số điện thoại đã được khách hàng khác sử dụng.";
    }

    TaiKhoan tk;
    tk.maTaiKhoan = nextMaTaiKhoan();
    tk.userName = user;
    tk.matKhau = pass;
    tk.role = ROLE_KHACH_HANG;
    tenants.addAccount(tk);

    if (existing) {
        // Khách đã từng thuê phòng: chỉ liên kết tài khoản với hồ sơ có sẵn
        KhachHang kh = *existing;
        kh.maTaiKhoan = tk.maTaiKhoan;
        tenants.updateCustomer(kh);
    } else {
        info.maTaiKhoan = tk.maTaiKhoan;
        err = addCustomer(info);
        if (!err.empty()) {
            tenants.removeAccount(tk.maTaiKhoan);
            return err;
        }
    }
    return "";
}

std::string AppData::addManagerAccount(const std::string& user, const std::string& pass, const std::string& tenQuanLy) {
    std::string err = validateAccount(user, pass);
    if (!err.empty()) return err;
    if (tenants.findAccount(user)) return "Tên đăng nhập đã tồn tại.";
    if (tenQuanLy.empty() || utf8Length(tenQuanLy) > MAX_TEN) return "Tên quản lý phải từ 1 đến 30 ký tự.";

    TaiKhoan tk;
    tk.maTaiKhoan = nextMaTaiKhoan();
    tk.userName = user;
    tk.matKhau = pass;
    tk.role = ROLE_QUAN_LY;
    tenants.addAccount(tk);

    QuanLy ql;
    ql.maQuanLy = nextMaQuanLy();
    ql.tenQuanLy = tenQuanLy;
    ql.maTaiKhoan = tk.maTaiKhoan;
    managers.push(ql);
    return "";
}

std::string AppData::changePassword(int maTaiKhoan, const std::string& oldPass, const std::string& newPass, bool checkOld) {
    TaiKhoan* tk = tenants.findAccount(maTaiKhoan);
    if (!tk) return "Không tìm thấy tài khoản.";
    if (checkOld && tk->matKhau != oldPass) return "Mật khẩu hiện tại không đúng.";
    std::string err = validateAccount(tk->userName, newPass);
    if (!err.empty()) return err;
    tk->matKhau = newPass;
    return "";
}

std::string AppData::deleteAccount(int maTaiKhoan, int currentUser) {
    TaiKhoan* tk = tenants.findAccount(maTaiKhoan);
    if (!tk) return "Không tìm thấy tài khoản.";
    if (maTaiKhoan == currentUser) return "Không thể xóa tài khoản đang đăng nhập.";
    if (tk->role == ROLE_QUAN_LY) {
        int soQuanLy = 0;
        DynArray<TaiKhoan*> all;
        tenants.getAccounts(all);
        for (int i = 0; i < all.size(); ++i)
            if (all[i]->role == ROLE_QUAN_LY) ++soQuanLy;
        if (soQuanLy <= 1) return "Hệ thống phải còn ít nhất một tài khoản quản lý.";
        QuanLy* ql = findManagerByAccount(maTaiKhoan);
        if (ql) ql->maTaiKhoan = 0;  // giữ hồ sơ quản lý vì phiếu cũ còn tham chiếu
    } else {
        KhachHang* kh = tenants.findByAccount(maTaiKhoan);
        if (kh) {
            KhachHang copy = *kh;
            copy.maTaiKhoan = 0;
            tenants.updateCustomer(copy);
        }
    }
    tenants.removeAccount(maTaiKhoan);
    return "";
}

// ================= Thuê / trả phòng =================

std::string AppData::rentRoom(PhieuThueTraPhong& p, const KhachHang& info) {
    RoomNode* room = rooms.find(p.maPhong);
    if (!room) return "Không tìm thấy phòng.";
    if (room->data.trangThai != PHONG_TRONG || activeRentalOfRoom(p.maPhong))
        return "Phòng " + std::to_string(p.maPhong) + " không còn trống.";
    if (p.ngayThue.empty()) return "Vui lòng chọn ngày thuê.";

    KhachHang* kh = tenants.findByCCCD(info.cccd);
    if (!kh) {
        KhachHang moi = info;
        moi.maTaiKhoan = 0;
        std::string err = addCustomer(moi);
        if (!err.empty()) return err;
        kh = tenants.findCustomer(moi.maKhachHang);
    }

    p.maPhieuTTP = nextMaPhieuTTP();
    p.tenKhachHang = kh->tenKhachHang;
    p.cccd = kh->cccd;
    p.sdt = kh->sdt;
    p.diaChi = kh->diaChi;
    p.ngayTra = Date();
    rentals.push(new PhieuThueTraPhong(p));
    room->data.trangThai = PHONG_DA_THUE;  // tự động đổi trạng thái phòng
    return "";
}

std::string AppData::returnRoom(int maPhieuTTP, const Date& ngayTra) {
    PhieuThueTraPhong* p = findRental(maPhieuTTP);
    if (!p) return "Không tìm thấy phiếu thuê.";
    if (!p->dangThue()) return "Phiếu thuê này đã được trả phòng.";
    if (ngayTra.empty() || ngayTra.compare(p->ngayThue) < 0) return "Ngày trả phải sau hoặc bằng ngày thuê.";
    p->ngayTra = ngayTra;
    RoomNode* room = rooms.find(p->maPhong);
    if (room) room->data.trangThai = PHONG_TRONG;
    return "";
}

std::string AppData::deleteRental(int maPhieuTTP) {
    for (int i = 0; i < bills.size(); ++i)
        if (bills[i]->maPhieuTTP == maPhieuTTP)
            return "Phiếu thuê đã có phiếu thu tiền nên không thể xóa.";
    for (int i = 0; i < rentals.size(); ++i) {
        PhieuThueTraPhong* p = rentals[i];
        if (p->maPhieuTTP != maPhieuTTP) continue;
        if (p->dangThue()) {
            RoomNode* room = rooms.find(p->maPhong);
            if (room) room->data.trangThai = PHONG_TRONG;
        }
        rentals.removeAt(i);
        delete p;
        return "";
    }
    return "Không tìm thấy phiếu thuê.";
}

// ================= Phiếu thu tiền =================

std::string AppData::createBill(PhieuThuTien& b) {
    PhieuThueTraPhong* p = findRental(b.maPhieuTTP);
    if (!p) return "Không tìm thấy phiếu thuê phòng.";
    if (b.thang < 1 || b.thang > 12) return "Tháng phải từ 1 đến 12.";
    if (b.nam < 2000 || b.nam > 2100) return "Năm không hợp lệ.";
    if (b.nam * 12 + b.thang < p->ngayThue.y * 12 + p->ngayThue.m)
        return "Kỳ thu tiền không được trước tháng bắt đầu thuê.";
    if (!p->ngayTra.empty() && b.nam * 12 + b.thang > p->ngayTra.y * 12 + p->ngayTra.m)
        return "Kỳ thu tiền không được sau tháng trả phòng.";
    if (billExists(b.maPhieuTTP, b.thang, b.nam))
        return "QD03: Phòng này đã có phiếu thu tiền cho tháng " + std::to_string(b.thang) + "/" +
               std::to_string(b.nam) + ".";
    if (b.soDien < 0 || b.soNuoc < 0 || b.tienWifi < 0) return "Chỉ số điện, nước, tiền wifi không được âm.";
    if (b.hanThanhToan.empty()) return "Vui lòng chọn hạn thanh toán.";

    RoomNode* room = rooms.find(p->maPhong);
    if (!room) return "Phòng của phiếu thuê không còn tồn tại.";
    KhachHang* kh = tenants.findByCCCD(p->cccd);
    if (!kh) return "Không tìm thấy khách hàng của phiếu thuê.";

    b.maPhieuThuTien = nextMaPhieuThu();
    b.tienPhong = room->data.giaThue;
    b.maKhachHang = kh->maKhachHang;
    b.maPhong = p->maPhong;
    BillingService::tinhHoaDon(b);

    PhieuThuTien* nb = new PhieuThuTien(b);
    nb->heapIndex = -1;
    bills.push(nb);
    if (!nb->daThanhToan) unpaid.push(nb);
    return "";
}

std::string AppData::payBill(int maPhieu) {
    PhieuThuTien* b = findBill(maPhieu);
    if (!b) return "Không tìm thấy phiếu thu.";
    if (b->daThanhToan) return "Phiếu thu này đã được thanh toán.";
    b->daThanhToan = true;
    unpaid.remove(b);
    return "";
}

std::string AppData::deleteBill(int maPhieu) {
    for (int i = 0; i < bills.size(); ++i) {
        PhieuThuTien* b = bills[i];
        if (b->maPhieuThuTien != maPhieu) continue;
        unpaid.remove(b);
        bills.removeAt(i);
        delete b;
        return "";
    }
    return "Không tìm thấy phiếu thu.";
}

// ================= Nạp dữ liệu =================

void AppData::loadBill(PhieuThuTien* b) {
    b->heapIndex = -1;
    bills.push(b);
    if (!b->daThanhToan) unpaid.push(b);
}

void AppData::finishLoading() {
    for (int i = 0; i < bills.size(); ++i) {
        PhieuThueTraPhong* p = findRental(bills[i]->maPhieuTTP);
        bills[i]->maPhong = p ? p->maPhong : 0;
    }
    if (tenants.accountCount() == 0) {
        // Dữ liệu trống: tạo sẵn tài khoản quản trị để đăng nhập lần đầu
        addManagerAccount("admin", "admin123", "Quản trị viên");
    }
}

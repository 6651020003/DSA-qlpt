#include "TenantHash.h"

// Băm nhân Knuth cho khóa số nguyên
unsigned long hashKey(int key) {
    return (unsigned long)((unsigned int)key * 2654435761u);
}

// FNV-1a 32-bit cho khóa chuỗi
unsigned long hashKey(const std::string& key) {
    unsigned int h = 2166136261u;
    for (size_t i = 0; i < key.size(); ++i) {
        h ^= (unsigned char)key[i];
        h *= 16777619u;
    }
    return h;
}

int nextPrime(int n) {
    if (n < 2) return 2;
    for (int p = n;; ++p) {
        bool prime = true;
        for (int d = 2; d * d <= p; ++d)
            if (p % d == 0) { prime = false; break; }
        if (prime) return p;
    }
}

TenantHash::TenantHash() {}

TenantHash::~TenantHash() { clear(); }

void TenantHash::clear() {
    m_byMa.forEach([](const int&, KhachHang* const& kh) { delete kh; });
    m_accById.forEach([](const int&, TaiKhoan* const& tk) { delete tk; });
    m_byMa.clear();
    m_byCCCD.clear();
    m_bySDT.clear();
    m_accById.clear();
    m_accByUser.clear();
}

// ---------------- Khách hàng ----------------

bool TenantHash::addCustomer(const KhachHang& kh) {
    if (m_byMa.find(kh.maKhachHang) || m_byCCCD.find(kh.cccd) || m_bySDT.find(kh.sdt)) return false;
    KhachHang* p = new KhachHang(kh);
    m_byMa.insert(p->maKhachHang, p);
    m_byCCCD.insert(p->cccd, p);
    m_bySDT.insert(p->sdt, p);
    return true;
}

bool TenantHash::updateCustomer(const KhachHang& kh) {
    KhachHang* cur = findCustomer(kh.maKhachHang);
    if (!cur) return false;
    if (kh.cccd != cur->cccd && m_byCCCD.find(kh.cccd)) return false;
    if (kh.sdt != cur->sdt && m_bySDT.find(kh.sdt)) return false;
    if (kh.cccd != cur->cccd) {
        m_byCCCD.remove(cur->cccd);
        m_byCCCD.insert(kh.cccd, cur);
    }
    if (kh.sdt != cur->sdt) {
        m_bySDT.remove(cur->sdt);
        m_bySDT.insert(kh.sdt, cur);
    }
    *cur = kh;
    return true;
}

bool TenantHash::removeCustomer(int maKhachHang) {
    KhachHang* kh = findCustomer(maKhachHang);
    if (!kh) return false;
    m_byCCCD.remove(kh->cccd);
    m_bySDT.remove(kh->sdt);
    m_byMa.remove(maKhachHang);
    delete kh;
    return true;
}

KhachHang* TenantHash::findCustomer(int maKhachHang) const {
    KhachHang** p = m_byMa.find(maKhachHang);
    return p ? *p : nullptr;
}

KhachHang* TenantHash::findByCCCD(const std::string& cccd) const {
    KhachHang** p = m_byCCCD.find(cccd);
    return p ? *p : nullptr;
}

KhachHang* TenantHash::findBySDT(const std::string& sdt) const {
    KhachHang** p = m_bySDT.find(sdt);
    return p ? *p : nullptr;
}

KhachHang* TenantHash::findByAccount(int maTaiKhoan) const {
    if (maTaiKhoan <= 0) return nullptr;
    KhachHang* found = nullptr;
    m_byMa.forEach([&](const int&, KhachHang* const& kh) {
        if (kh->maTaiKhoan == maTaiKhoan) found = kh;
    });
    return found;
}

void TenantHash::getCustomers(DynArray<KhachHang*>& out) const {
    out.clear();
    m_byMa.forEach([&](const int&, KhachHang* const& kh) { out.push(kh); });
}

int TenantHash::maxCustomerId() const {
    int mx = 0;
    m_byMa.forEach([&](const int& k, KhachHang* const&) { if (k > mx) mx = k; });
    return mx;
}

// ---------------- Tài khoản ----------------

bool TenantHash::addAccount(const TaiKhoan& tk) {
    if (m_accById.find(tk.maTaiKhoan) || m_accByUser.find(tk.userName)) return false;
    TaiKhoan* p = new TaiKhoan(tk);
    m_accById.insert(p->maTaiKhoan, p);
    m_accByUser.insert(p->userName, p);
    return true;
}

bool TenantHash::removeAccount(int maTaiKhoan) {
    TaiKhoan* tk = findAccount(maTaiKhoan);
    if (!tk) return false;
    m_accByUser.remove(tk->userName);
    m_accById.remove(maTaiKhoan);
    delete tk;
    return true;
}

TaiKhoan* TenantHash::findAccount(int maTaiKhoan) const {
    TaiKhoan** p = m_accById.find(maTaiKhoan);
    return p ? *p : nullptr;
}

TaiKhoan* TenantHash::findAccount(const std::string& userName) const {
    TaiKhoan** p = m_accByUser.find(userName);
    return p ? *p : nullptr;
}

void TenantHash::getAccounts(DynArray<TaiKhoan*>& out) const {
    out.clear();
    m_accById.forEach([&](const int&, TaiKhoan* const& tk) { out.push(tk); });
}

int TenantHash::maxAccountId() const {
    int mx = 0;
    m_accById.forEach([&](const int& k, TaiKhoan* const&) { if (k > mx) mx = k; });
    return mx;
}

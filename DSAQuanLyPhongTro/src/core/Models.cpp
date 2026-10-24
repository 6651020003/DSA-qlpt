#include "Models.h"
#include <cstdio>
#include <ctime>

// Thuật toán days_from_civil / civil_from_days (lịch Gregory)
long Date::toDays() const {
    int yy = y - (m <= 2 ? 1 : 0);
    long era = (yy >= 0 ? yy : yy - 399) / 400;
    unsigned yoe = (unsigned)(yy - era * 400);
    unsigned mp = (unsigned)(m > 2 ? m - 3 : m + 9);
    unsigned doy = (153 * mp + 2) / 5 + (unsigned)d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

Date Date::fromDays(long z) {
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long yy = (long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    unsigned dd = doy - (153 * mp + 2) / 5 + 1;
    unsigned mm = mp < 10 ? mp + 3 : mp - 9;
    return Date((int)(yy + (mm <= 2 ? 1 : 0)), (int)mm, (int)dd);
}

int Date::compare(const Date& o) const {
    if (y != o.y) return y < o.y ? -1 : 1;
    if (m != o.m) return m < o.m ? -1 : 1;
    if (d != o.d) return d < o.d ? -1 : 1;
    return 0;
}

std::string Date::toString() const {
    if (empty()) return "";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return buf;
}

std::string Date::toDisplay() const {
    if (empty()) return "";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d", d, m, y);
    return buf;
}

bool Date::isValid(int y, int m, int d) {
    if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1) return false;
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = days[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) maxDay = 29;
    return d <= maxDay;
}

bool Date::parse(const std::string& s, Date& out) {
    std::string t = trimStr(s);
    int a = 0, b = 0, c = 0;
    if (std::sscanf(t.c_str(), "%d-%d-%d", &a, &b, &c) == 3 && isValid(a, b, c)) {
        out = Date(a, b, c);
        return true;
    }
    if (std::sscanf(t.c_str(), "%d/%d/%d", &a, &b, &c) == 3 && isValid(c, b, a)) {
        out = Date(c, b, a);
        return true;
    }
    return false;
}

Date Date::today() {
    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    return Date(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

const char* tenTrangThaiPhong(int trangThai) {
    return trangThai == PHONG_DA_THUE ? "Đã thuê" : "Trống";
}

const char* tenVaiTro(int role) {
    return role == ROLE_QUAN_LY ? "Quản lý" : "Khách hàng";
}

int utf8Length(const std::string& s) {
    int n = 0;
    for (size_t i = 0; i < s.size(); ++i)
        if (((unsigned char)s[i] & 0xC0) != 0x80) ++n;
    return n;
}

bool isDigits(const std::string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

std::string trimStr(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

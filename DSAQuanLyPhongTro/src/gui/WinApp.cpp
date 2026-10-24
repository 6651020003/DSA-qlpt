#include "WinApp.h"
#include "Controls.h"
#include "../services/BillingService.h"
#include <commdlg.h>

using namespace ui;

namespace {

const wchar_t* MAIN_CLASS = L"QLPT_MainWindow";
const wchar_t* APP_TITLE = L"Quản lý Phòng trọ & Hóa đơn Dịch vụ";

enum MenuId {
    IDM_SAVE = 101, IDM_REFRESH, IDM_PASSWORD, IDM_LOGOUT, IDM_EXIT,
    IDM_ROOMS = 110, IDM_ROOM_ADD, IDM_ROOM_IMPORT,
    IDM_CUSTOMERS = 120, IDM_CUSTOMER_ADD,
    IDM_RENTALS = 130, IDM_RENT, IDM_RETURN,
    IDM_BILLS = 140, IDM_BILL_ADD,
    IDM_UNPAID = 150, IDM_REVENUE, IDM_LOYAL,
    IDM_ACCOUNTS = 160,
    IDM_MY = 170, IDM_MY_INFO,
    IDM_ABOUT = 180
};

enum CtrlId {
    IDC_LIST = 1000, IDC_TITLE, IDC_STATUS,
    IDB_ADD = 1100, IDB_EDIT, IDB_DELETE, IDB_IMPORT, IDB_PREV, IDB_NEXT, IDB_RENT, IDB_RETURN, IDB_PAY,
    IDB_BILL, IDB_DETAIL, IDB_PASS, IDB_INFO, IDB_SORT, IDB_SEARCH, IDB_RESET,
    IDF_E1 = 1200, IDF_E2, IDF_E3, IDF_E4, IDF_CHECK, IDF_C1, IDF_C2, IDF_C3, IDF_INFO
};

const int ID_REGISTER = 10;
const int ID_BROWSE = 11;

const int MARGIN = 12;
const int ROW_H = 28;
const int ROW0_Y = 50;
const int ROW1_Y = 84;

std::wstring kyThu(int thang, int nam) {
    wchar_t buf[16];
    swprintf(buf, 16, L"%02d/%04d", thang, nam);
    return buf;
}

}  // namespace

// =====================================================================
//                              Khởi tạo
// =====================================================================

WinApp::WinApp(HINSTANCE inst, AppData& db, FileStorage& storage)
    : m_inst(inst), m_db(db), m_storage(storage), m_hwnd(NULL), m_title(NULL), m_list(NULL), m_status(NULL),
      m_accel(NULL), m_userId(0), m_role(ROLE_KHACH_HANG), m_maQL(0), m_logout(false), m_view(V_NONE),
      m_listTop(ROW1_Y), m_defBtn(0), m_roomFiltered(false), m_curRoom(0), m_roomSortKey(ROOM_BY_MA),
      m_roomSortAsc(true), m_roomSorted(false), m_billSortKey(BILL_BY_THANG), m_billSortAsc(false) {
    m_x[0] = m_x[1] = MARGIN;
}

int WinApp::run(int nCmdShow) {
    initFonts();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = m_inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = MAIN_CLASS;
    wc.hIcon = LoadIcon(m_inst, MAKEINTRESOURCE(1));
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    ACCEL acc[] = {
        {FCONTROL | FVIRTKEY, 'S', IDM_SAVE},
        {FVIRTKEY, VK_F5, IDM_REFRESH},
    };
    m_accel = CreateAcceleratorTableW(acc, 2);

    for (;;) {
        if (!login()) break;
        m_logout = false;
        createMainWindow(nCmdShow);
        nCmdShow = SW_SHOW;

        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0) > 0) {
            if (m_hwnd && TranslateAcceleratorW(m_hwnd, m_accel, &msg)) continue;
            if (m_hwnd && IsDialogMessageW(m_hwnd, &msg)) continue;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!m_logout) break;
    }
    DestroyAcceleratorTable(m_accel);
    return 0;
}

bool WinApp::login() {
    std::wstring lastUser;
    for (;;) {
        FormDialog f(L"Đăng nhập - Quản lý Phòng trọ", 440);
        int iUser = f.add(L"Tên đăng nhập", FT_TEXT, lastUser);
        int iPass = f.add(L"Mật khẩu", FT_PASSWORD);
        f.setNote(L"Chưa có tài khoản? Khách hàng có thể bấm \"Đăng ký\" để tạo tài khoản mới.");
        f.setOkText(L"Đăng nhập");
        f.setCancelText(L"Thoát");
        f.addButton(ID_REGISTER, L"Đăng ký...");

        TaiKhoan* acc = nullptr;
        f.onSubmit = [&](FormDialog& d, int id) -> std::wstring {
            if (id == ID_REGISTER) return L"";
            std::wstring u = trim(d.text(iUser));
            if (u.empty()) return L"Vui lòng nhập tên đăng nhập.";
            acc = m_db.login(U8(u), U8(d.text(iPass)));
            if (!acc) return L"Sai tên đăng nhập hoặc mật khẩu.";
            return L"";
        };

        int r = f.show(NULL);
        lastUser = trim(f.text(iUser));
        if (r == ID_REGISTER) {
            registerDialog();
            continue;
        }
        if (r != IDOK || !acc) return false;

        m_userId = acc->maTaiKhoan;
        m_role = acc->role;
        QuanLy* ql = m_db.findManagerByAccount(m_userId);
        m_maQL = ql ? ql->maQuanLy : 0;
        return true;
    }
}

void WinApp::registerDialog() {
    FormDialog f(L"Đăng ký tài khoản khách hàng", 500);
    int iUser = f.add(L"Tên đăng nhập", FT_TEXT);
    int iPass = f.add(L"Mật khẩu", FT_PASSWORD);
    int iPass2 = f.add(L"Nhập lại mật khẩu", FT_PASSWORD);
    int iTen = f.add(L"Tên khách hàng", FT_TEXT);
    int iCccd = f.add(L"Căn cước công dân", FT_TEXT);
    int iDc = f.add(L"Địa chỉ", FT_TEXT);
    int iSdt = f.add(L"Số điện thoại", FT_TEXT);
    f.setNote(L"Nếu bạn đã từng thuê phòng, nhập đúng CCCD để liên kết với hồ sơ cũ.");
    f.setOkText(L"Đăng ký");
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        if (d.text(iPass) != d.text(iPass2)) return L"Mật khẩu nhập lại không khớp.";
        KhachHang kh;
        kh.tenKhachHang = U8(trim(d.text(iTen)));
        kh.cccd = U8(trim(d.text(iCccd)));
        kh.diaChi = U8(trim(d.text(iDc)));
        kh.sdt = U8(trim(d.text(iSdt)));
        std::string err = m_db.registerCustomer(U8(trim(d.text(iUser))), U8(d.text(iPass)), kh);
        return W(err);
    };
    if (f.show(NULL) == IDOK) {
        commit();
        info(NULL, L"Đăng ký thành công. Hãy đăng nhập bằng tài khoản vừa tạo.");
    }
}

void WinApp::createMainWindow(int nCmdShow) {
    m_hwnd = CreateWindowExW(WS_EX_CONTROLPARENT, MAIN_CLASS, APP_TITLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1240, 740, NULL, buildMenu(), m_inst, this);

    m_title = label(m_hwnd, L"", MARGIN, 10, 700, 32);
    SendMessageW(m_title, WM_SETFONT, (WPARAM)titleFont(), TRUE);
    m_status = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hwnd,
                               (HMENU)(INT_PTR)IDC_STATUS, m_inst, NULL);
    SendMessageW(m_status, WM_SETFONT, (WPARAM)font(), TRUE);

    m_view = V_NONE;
    m_curRoom = 0;
    m_roomFiltered = false;
    switchView(isManager() ? V_ROOMS : V_MY);
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

HMENU WinApp::buildMenu() {
    HMENU bar = CreateMenu();

    HMENU sys = CreatePopupMenu();
    AppendMenuW(sys, MF_STRING, IDM_SAVE, L"&Lưu dữ liệu\tCtrl+S");
    AppendMenuW(sys, MF_STRING, IDM_REFRESH, L"Làm &mới\tF5");
    AppendMenuW(sys, MF_STRING, IDM_PASSWORD, L"Đổi &mật khẩu...");
    AppendMenuW(sys, MF_SEPARATOR, 0, NULL);
    AppendMenuW(sys, MF_STRING, IDM_LOGOUT, L"Đăng &xuất");
    AppendMenuW(sys, MF_STRING, IDM_EXIT, L"&Thoát");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)sys, L"&Hệ thống");

    HMENU room = CreatePopupMenu();
    AppendMenuW(room, MF_STRING, IDM_ROOMS, L"&Danh sách / tìm kiếm phòng");
    if (isManager()) {
        AppendMenuW(room, MF_STRING, IDM_ROOM_ADD, L"&Thêm phòng...");
        AppendMenuW(room, MF_STRING, IDM_ROOM_IMPORT, L"&Import từ CSV...");
    }
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)room, L"&Phòng trọ");

    if (isManager()) {
        HMENU cus = CreatePopupMenu();
        AppendMenuW(cus, MF_STRING, IDM_CUSTOMERS, L"&Danh sách khách hàng");
        AppendMenuW(cus, MF_STRING, IDM_CUSTOMER_ADD, L"&Thêm khách hàng...");
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)cus, L"&Khách hàng");

        HMENU rent = CreatePopupMenu();
        AppendMenuW(rent, MF_STRING, IDM_RENTALS, L"&Danh sách phiếu thuê/trả");
        AppendMenuW(rent, MF_STRING, IDM_RENT, L"Lập phiếu &thuê phòng...");
        AppendMenuW(rent, MF_STRING, IDM_RETURN, L"Lập phiếu t&rả phòng...");
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)rent, L"&Thuê - Trả");

        HMENU bill = CreatePopupMenu();
        AppendMenuW(bill, MF_STRING, IDM_BILLS, L"&Danh sách phiếu thu");
        AppendMenuW(bill, MF_STRING, IDM_BILL_ADD, L"&Lập phiếu thu tiền...");
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)bill, L"Hóa đ&ơn");

        HMENU stat = CreatePopupMenu();
        AppendMenuW(stat, MF_STRING, IDM_REVENUE, L"&Doanh thu");
        AppendMenuW(stat, MF_STRING, IDM_UNPAID, L"&Chưa thanh toán");
        AppendMenuW(stat, MF_STRING, IDM_LOYAL, L"&Khách hàng thân thiết");
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)stat, L"Th&ống kê");

        HMENU acc = CreatePopupMenu();
        AppendMenuW(acc, MF_STRING, IDM_ACCOUNTS, L"&Danh sách tài khoản");
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)acc, L"T&ài khoản");
    } else {
        HMENU my = CreatePopupMenu();
        AppendMenuW(my, MF_STRING, IDM_MY, L"&Phòng && hóa đơn của tôi");
        AppendMenuW(my, MF_STRING, IDM_MY_INFO, L"&Cập nhật thông tin cá nhân...");
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)my, L"&Của tôi");
    }

    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, IDM_ABOUT, L"&Giới thiệu");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)help, L"Trợ &giúp");
    return bar;
}

// =====================================================================
//                         Xử lý thông điệp
// =====================================================================

LRESULT CALLBACK WinApp::wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    WinApp* self;
    if (msg == WM_NCCREATE) {
        self = (WinApp*)((CREATESTRUCTW*)lp)->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hwnd = h;
    } else {
        self = (WinApp*)GetWindowLongPtrW(h, GWLP_USERDATA);
    }
    return self ? self->handle(msg, wp, lp) : DefWindowProcW(h, msg, wp, lp);
}

LRESULT WinApp::handle(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        layout();
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize.x = 1100;
        mm->ptMinTrackSize.y = 560;
        return 0;
    }
    case WM_COMMAND:
        onCommand(LOWORD(wp), HIWORD(wp));
        return 0;
    case WM_NOTIFY:
        return onNotify((NMHDR*)lp);
    case DM_GETDEFID:
        return m_defBtn ? MAKELRESULT(m_defBtn, DC_HASDEFID) : 0;
    case WM_CLOSE:
        commit();
        DestroyWindow(m_hwnd);
        return 0;
    case WM_DESTROY:
        m_ctrls.clear();
        m_list = NULL;
        m_hwnd = NULL;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(m_hwnd, msg, wp, lp);
}

void WinApp::layout() {
    if (!m_hwnd || !m_status) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int cw = rc.right, ch = rc.bottom;

    SendMessageW(m_status, WM_SIZE, 0, 0);
    RECT sr;
    GetWindowRect(m_status, &sr);
    int sh = sr.bottom - sr.top;
    int parts[3] = {cw * 45 / 100, cw - 280, -1};
    SendMessageW(m_status, SB_SETPARTS, 3, (LPARAM)parts);

    MoveWindow(m_title, MARGIN, 10, cw - 2 * MARGIN, 32, TRUE);
    if (m_list) MoveWindow(m_list, MARGIN, m_listTop, cw - 2 * MARGIN, ch - m_listTop - sh - 8, TRUE);
}

bool WinApp::commit() {
    std::string err;
    if (!m_storage.saveAll(m_db, err)) {
        warn(m_hwnd, W(err));
        return false;
    }
    return true;
}

void WinApp::setStatus(const std::wstring& text) {
    if (m_status) SendMessageW(m_status, SB_SETTEXTW, 0, (LPARAM)text.c_str());
}

void WinApp::setDetail(const std::wstring& text) {
    if (m_status) SendMessageW(m_status, SB_SETTEXTW, 1, (LPARAM)text.c_str());
}

std::wstring WinApp::managerName(int maQL) const {
    QuanLy* ql = m_db.findManager(maQL);
    return ql ? W(ql->tenQuanLy) : (maQL > 0 ? L"QL " + num(maQL) : L"");
}

std::wstring WinApp::customerName(int maKH) const {
    KhachHang* kh = m_db.tenants.findCustomer(maKH);
    return kh ? W(kh->tenKhachHang) : L"(KH " + num(maKH) + L")";
}

void WinApp::onCommand(int id, int code) {
    // ----- Menu / phím tắt -----
    switch (id) {
    case IDM_SAVE:
        if (commit()) setStatus(L"Đã lưu dữ liệu vào thư mục data.");
        return;
    case IDM_REFRESH: refresh(); return;
    case IDM_PASSWORD: changePasswordDialog(m_userId, true); return;
    case IDM_LOGOUT:
        if (!confirm(m_hwnd, L"Đăng xuất khỏi hệ thống?")) return;
        commit();
        m_logout = true;
        DestroyWindow(m_hwnd);
        return;
    case IDM_EXIT: SendMessageW(m_hwnd, WM_CLOSE, 0, 0); return;
    case IDM_ROOMS: switchView(V_ROOMS); return;
    case IDM_ROOM_ADD: switchView(V_ROOMS); roomDialog(0); return;
    case IDM_ROOM_IMPORT: switchView(V_ROOMS); importRooms(); return;
    case IDM_CUSTOMERS: switchView(V_CUSTOMERS); return;
    case IDM_CUSTOMER_ADD: switchView(V_CUSTOMERS); customerDialog(0); return;
    case IDM_RENTALS: switchView(V_RENTALS); return;
    case IDM_RENT: rentDialog(0); return;
    case IDM_RETURN: returnDialog(0); return;
    case IDM_BILLS: switchView(V_BILLS); return;
    case IDM_BILL_ADD: billDialog(0); return;
    case IDM_UNPAID: switchView(V_UNPAID); return;
    case IDM_REVENUE: switchView(V_REVENUE); return;
    case IDM_LOYAL: switchView(V_LOYAL); return;
    case IDM_ACCOUNTS: switchView(V_ACCOUNTS); return;
    case IDM_MY: switchView(V_MY); return;
    case IDM_MY_INFO: {
        KhachHang* kh = m_db.tenants.findByAccount(m_userId);
        if (kh) customerDialog(kh->maKhachHang);
        else warn(m_hwnd, L"Tài khoản chưa liên kết hồ sơ khách hàng.");
        return;
    }
    case IDM_ABOUT: aboutDialog(); return;
    }

    // ----- Control trên màn hình -----
    if (code == CBN_SELCHANGE && (id == IDF_C1 || id == IDF_C2 || id == IDF_C3)) {
        if (m_view == V_RENTALS || m_view == V_BILLS) refresh();
        return;
    }
    if (code != BN_CLICKED || id < IDB_ADD || id > IDF_INFO) return;

    switch (m_view) {
    case V_ROOMS: roomCommand(id); break;
    case V_CUSTOMERS: customerCommand(id); break;
    case V_RENTALS: rentalCommand(id); break;
    case V_BILLS: billCommand(id); break;
    case V_UNPAID:
        if (id == IDB_SEARCH) refreshUnpaid();
        else billCommand(id);
        break;
    case V_REVENUE:
        if (id == IDB_SEARCH) refreshRevenue();
        break;
    case V_LOYAL:
        if (id == IDB_SEARCH) refreshLoyal();
        break;
    case V_ACCOUNTS: accountCommand(id); break;
    case V_MY:
        if (id == IDB_INFO) onCommand(IDM_MY_INFO, 0);
        else if (id == IDB_PASS) changePasswordDialog(m_userId, true);
        else if (id == IDB_DETAIL) {
            LPARAM sel = lvSelectedData(m_list);
            if (sel > 0) billDetail((int)sel);
            else warn(m_hwnd, L"Hãy chọn một phiếu thu.");
        }
        break;
    default: break;
    }
}

LRESULT WinApp::onNotify(NMHDR* nm) {
    if (nm->idFrom != IDC_LIST) return 0;
    if (nm->code == NM_DBLCLK) {
        LPARAM sel = lvSelectedData(m_list);
        if (sel <= 0) return 0;
        switch (m_view) {
        case V_ROOMS:
            if (isManager()) roomDialog((int)sel);
            break;
        case V_CUSTOMERS: customerDialog((int)sel); break;
        case V_RENTALS: {
            PhieuThueTraPhong* p = m_db.findRental((int)sel);
            if (p && p->dangThue()) returnDialog(p->maPhieuTTP);
            break;
        }
        case V_BILLS:
        case V_UNPAID:
        case V_MY: billDetail((int)sel); break;
        default: break;
        }
    } else if (nm->code == LVN_ITEMCHANGED && m_view == V_ROOMS) {
        NMLISTVIEW* lv = (NMLISTVIEW*)nm;
        if ((lv->uNewState & LVIS_SELECTED) && !(lv->uOldState & LVIS_SELECTED)) {
            m_curRoom = (int)lv->lParam;
            showRoomDetail(m_curRoom);
        }
    } else if (nm->code == LVN_COLUMNCLICK) {
        int col = ((NMLISTVIEW*)nm)->iSubItem;
        if (m_view == V_ROOMS && col <= 2) {
            int key = col == 0 ? ROOM_BY_MA : (col == 1 ? ROOM_BY_GIA : ROOM_BY_DIEN_TICH);
            m_roomSortAsc = (key == m_roomSortKey) ? !m_roomSortAsc : true;
            m_roomSortKey = key;
            SendMessageW(ctl(IDF_C1), CB_SETCURSEL, key, 0);
            SendMessageW(ctl(IDF_C2), CB_SETCURSEL, m_roomSortAsc ? 0 : 1, 0);
            roomCommand(IDB_SORT);
        } else if (m_view == V_BILLS && (col == 0 || col == 3 || col == 10)) {
            int key = col == 0 ? BILL_BY_MA : (col == 3 ? BILL_BY_THANG : BILL_BY_TONG_TIEN);
            m_billSortAsc = (key == m_billSortKey) ? !m_billSortAsc : true;
            m_billSortKey = key;
            SendMessageW(ctl(IDF_C2), CB_SETCURSEL, key, 0);
            SendMessageW(ctl(IDF_C3), CB_SETCURSEL, m_billSortAsc ? 0 : 1, 0);
            refreshBills();
        }
    }
    return 0;
}

// =====================================================================
//                        Dựng các màn hình
// =====================================================================

HWND WinApp::track(HWND h) {
    m_ctrls.push_back(h);
    return h;
}

HWND WinApp::tbButton(int row, int id, const wchar_t* text, int w) {
    HWND h = button(m_hwnd, id, text, m_x[row], row == 0 ? ROW0_Y : ROW1_Y, w, ROW_H);
    m_x[row] += w + 6;
    return track(h);
}

HWND WinApp::tbLabel(int row, const wchar_t* text, int w) {
    HWND h = label(m_hwnd, text, m_x[row], (row == 0 ? ROW0_Y : ROW1_Y) + 6, w, 20);
    m_x[row] += w + 4;
    return track(h);
}

HWND WinApp::tbEdit(int row, int id, int w, const std::wstring& text) {
    HWND h = edit(m_hwnd, id, m_x[row], (row == 0 ? ROW0_Y : ROW1_Y) + 2, w, 24);
    setText(h, text);
    m_x[row] += w + 8;
    return track(h);
}

HWND WinApp::tbCombo(int row, int id, int w, const std::vector<std::wstring>& items, int sel) {
    HWND h = combo(m_hwnd, id, m_x[row], (row == 0 ? ROW0_Y : ROW1_Y) + 2, w, items, sel);
    m_x[row] += w + 8;
    return track(h);
}

HWND WinApp::tbCheck(int row, int id, const wchar_t* text, int w) {
    HWND h = checkbox(m_hwnd, id, text, m_x[row], (row == 0 ? ROW0_Y : ROW1_Y) + 2, w, 24);
    m_x[row] += w + 8;
    return track(h);
}

void WinApp::clearView() {
    for (HWND h : m_ctrls) DestroyWindow(h);
    m_ctrls.clear();
    if (m_list) DestroyWindow(m_list);
    m_list = NULL;
    m_x[0] = m_x[1] = MARGIN;
    m_defBtn = 0;
}

void WinApp::switchView(View v) {
    if (!m_hwnd) return;
    clearView();
    m_view = v;
    m_list = listView(m_hwnd, IDC_LIST);
    setStatus(L"");
    setDetail(L"");

    switch (v) {
    case V_ROOMS: buildRooms(); break;
    case V_CUSTOMERS: buildCustomers(); break;
    case V_RENTALS: buildRentals(); break;
    case V_BILLS: buildBills(); break;
    case V_UNPAID: buildUnpaid(); break;
    case V_REVENUE: buildRevenue(); break;
    case V_LOYAL: buildLoyal(); break;
    case V_ACCOUNTS: buildAccounts(); break;
    case V_MY: buildMy(); break;
    default: break;
    }
    m_listTop = (m_x[1] > MARGIN ? ROW1_Y : ROW0_Y) + ROW_H + 10;

    layout();  // chia ô thanh trạng thái trước khi ghi chữ vào ô thứ 3
    TaiKhoan* tk = m_db.tenants.findAccount(m_userId);
    std::wstring who = tk ? L"Người dùng: " + W(tk->userName) + L" (" + W(tenVaiTro(tk->role)) + L")" : L"";
    SendMessageW(m_status, SB_SETTEXTW, 2, (LPARAM)who.c_str());
    refresh();
    RedrawWindow(m_hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void WinApp::refresh() {
    switch (m_view) {
    case V_ROOMS: refreshRooms(); break;
    case V_CUSTOMERS: refreshCustomers(); break;
    case V_RENTALS: refreshRentals(); break;
    case V_BILLS: refreshBills(); break;
    case V_UNPAID: refreshUnpaid(); break;
    case V_REVENUE: refreshRevenue(); break;
    case V_LOYAL: refreshLoyal(); break;
    case V_ACCOUNTS: refreshAccounts(); break;
    case V_MY: refreshMy(); break;
    default: break;
    }
}

// =====================================================================
//                         Màn hình Phòng trọ
// =====================================================================

void WinApp::buildRooms() {
    setText(m_title, isManager() ? L"Quản lý phòng trọ" : L"Danh sách phòng trọ");
    if (isManager()) {
        tbButton(0, IDB_ADD, L"Thêm phòng", 96);
        tbButton(0, IDB_EDIT, L"Sửa", 64);
        tbButton(0, IDB_DELETE, L"Xóa", 64);
        tbButton(0, IDB_RENT, L"Cho thuê...", 92);
        tbButton(0, IDB_IMPORT, L"Import CSV...", 104);
        tbGap(0, 14);
    }
    tbButton(0, IDB_PREV, L"◀ Trước", 76);
    tbButton(0, IDB_NEXT, L"Sau ▶", 76);
    tbGap(0, 14);
    tbLabel(0, L"Sắp xếp:", 58);
    tbCombo(0, IDF_C1, 104, {L"Mã phòng", L"Giá thuê", L"Diện tích"}, m_roomSortKey);
    tbCombo(0, IDF_C2, 96, {L"Tăng dần", L"Giảm dần"}, m_roomSortAsc ? 0 : 1);
    tbButton(0, IDB_SORT, L"Sắp xếp", 80);

    tbLabel(1, L"Giá thuê từ", 72);
    tbEdit(1, IDF_E1, 100, m_roomFilter.giaMin >= 0 && m_roomFiltered ? money(m_roomFilter.giaMin) : L"");
    tbLabel(1, L"đến", 28);
    tbEdit(1, IDF_E2, 100, m_roomFilter.giaMax >= 0 && m_roomFiltered ? money(m_roomFilter.giaMax) : L"");
    tbGap(1, 8);
    tbLabel(1, L"Diện tích từ", 78);
    tbEdit(1, IDF_E3, 52, m_roomFilter.dtMin >= 0 && m_roomFiltered ? num(m_roomFilter.dtMin) : L"");
    tbLabel(1, L"đến", 28);
    tbEdit(1, IDF_E4, 52, m_roomFilter.dtMax >= 0 && m_roomFiltered ? num(m_roomFilter.dtMax) : L"");
    tbGap(1, 8);
    HWND chk = tbCheck(1, IDF_CHECK, L"Chỉ phòng trống", 130);
    if (m_roomFiltered && m_roomFilter.chiPhongTrong) SendMessageW(chk, BM_SETCHECK, BST_CHECKED, 0);
    tbButton(1, IDB_SEARCH, L"Tìm kiếm", 86);
    tbButton(1, IDB_RESET, L"Bỏ lọc", 72);
    m_defBtn = IDB_SEARCH;

    lvSetColumns(m_list, {{L"Mã phòng", 90, false},
                          {L"Giá thuê (đ/tháng)", 140, true},
                          {L"Diện tích (m²)", 110, true},
                          {L"Vật chất", 300, false},
                          {L"Trạng thái", 100, false},
                          {L"Hình", 200, false}});
}

void WinApp::refreshRooms() {
    DynArray<PhongTro*> rows;
    if (m_roomFiltered) {
        Sorter::searchRooms(m_db.rooms, m_roomFilter, rows);
        if (m_roomSorted) Sorter::quickSortRooms(rows, (RoomSortKey)m_roomSortKey, m_roomSortAsc);
    } else {
        for (RoomNode* n = m_db.rooms.head(); n; n = n->next) rows.push(&n->data);
    }

    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const PhongTro* p = rows[i];
        lvAddRow(m_list,
                 {num(p->maPhong), money(p->giaThue), num(p->dienTich), W(p->vatChat), W(tenTrangThaiPhong(p->trangThai)),
                  W(p->hinh)},
                 p->maPhong);
    }
    lvEndUpdate(m_list);
    if (m_curRoom) lvSelectByData(m_list, m_curRoom);

    int trong = m_db.rooms.countByStatus(PHONG_TRONG);
    std::wstring s = m_roomFiltered ? L"Tìm thấy " + num(rows.size()) + L"/" + num(m_db.rooms.size()) +
                                          L" phòng (QuickSort + Binary Search theo giá)"
                                    : L"Tổng " + num(m_db.rooms.size()) + L" phòng";
    s += L"  |  Trống: " + num(trong) + L"  |  Đã thuê: " + num(m_db.rooms.size() - trong);
    setStatus(s);
    if (m_roomFiltered && rows.size() == 0) setDetail(L"Không có phòng nào phù hợp với tiêu chí tìm kiếm.");
}

void WinApp::showRoomDetail(int maPhong) {
    RoomNode* n = m_db.rooms.find(maPhong);
    if (!n) {
        setDetail(L"");
        return;
    }
    std::wstring s = L"Phòng " + num(n->data.maPhong) + L": " + money(n->data.giaThue) + L"đ, " +
                     num(n->data.dienTich) + L"m², " + W(tenTrangThaiPhong(n->data.trangThai));
    s += L"   [trước: " + (n->prev ? num(n->prev->data.maPhong) : std::wstring(L"—")) +
         L" | sau: " + (n->next ? num(n->next->data.maPhong) : std::wstring(L"—")) + L"]";
    setDetail(s);
}

void WinApp::roomCommand(int id) {
    int sel = (int)lvSelectedData(m_list);
    switch (id) {
    case IDB_ADD: roomDialog(0); break;
    case IDB_EDIT:
        if (sel > 0) roomDialog(sel);
        else warn(m_hwnd, L"Hãy chọn phòng cần sửa.");
        break;
    case IDB_DELETE: {
        if (sel <= 0) {
            warn(m_hwnd, L"Hãy chọn phòng cần xóa.");
            break;
        }
        if (!confirm(m_hwnd, L"Xóa phòng " + num(sel) + L"? Thao tác không thể hoàn tác.")) break;
        std::string err = m_db.deleteRoom(sel);
        if (!err.empty()) {
            warn(m_hwnd, W(err));
            break;
        }
        if (m_curRoom == sel) m_curRoom = 0;
        commit();
        refreshRooms();
        setDetail(L"Đã xóa phòng " + num(sel) + L".");
        break;
    }
    case IDB_RENT: rentDialog(sel > 0 ? sel : 0); break;
    case IDB_IMPORT: importRooms(); break;
    case IDB_PREV:
    case IDB_NEXT: {
        // Duyệt tiến / lùi bằng con trỏ next / prev của DSLK đôi
        RoomNode* n = m_curRoom ? m_db.rooms.find(m_curRoom) : nullptr;
        if (!n) n = id == IDB_NEXT ? m_db.rooms.head() : m_db.rooms.tail();
        else n = id == IDB_NEXT ? n->next : n->prev;
        if (!n) {
            setDetail(id == IDB_NEXT ? L"Đã đến cuối danh sách." : L"Đã đến đầu danh sách.");
            break;
        }
        m_curRoom = n->data.maPhong;
        ListView_SetItemState(m_list, -1, 0, LVIS_SELECTED);
        lvSelectByData(m_list, m_curRoom);
        showRoomDetail(m_curRoom);
        SetFocus(m_list);
        break;
    }
    case IDB_SORT:
        m_roomSortKey = comboSel(ctl(IDF_C1));
        m_roomSortAsc = comboSel(ctl(IDF_C2)) == 0;
        m_roomSorted = true;
        if (!m_roomFiltered) {
            Sorter::mergeSortRooms(m_db.rooms, (RoomSortKey)m_roomSortKey, m_roomSortAsc);
            commit();
        }
        refreshRooms();
        setDetail(m_roomFiltered ? L"Đã sắp xếp kết quả tìm kiếm bằng QuickSort."
                                 : L"Đã sắp xếp danh sách liên kết đôi bằng MergeSort.");
        break;
    case IDB_SEARCH: {
        RoomFilter f;
        long long v;
        std::wstring t;
        if (!(t = trim(getText(ctl(IDF_E1)))).empty()) {
            if (!parseNumber(t, v)) { warn(m_hwnd, L"Giá thuê \"từ\" không hợp lệ."); break; }
            f.giaMin = v;
        }
        if (!(t = trim(getText(ctl(IDF_E2)))).empty()) {
            if (!parseNumber(t, v)) { warn(m_hwnd, L"Giá thuê \"đến\" không hợp lệ."); break; }
            f.giaMax = v;
        }
        if (!(t = trim(getText(ctl(IDF_E3)))).empty()) {
            if (!parseNumber(t, v)) { warn(m_hwnd, L"Diện tích \"từ\" không hợp lệ."); break; }
            f.dtMin = (int)v;
        }
        if (!(t = trim(getText(ctl(IDF_E4)))).empty()) {
            if (!parseNumber(t, v)) { warn(m_hwnd, L"Diện tích \"đến\" không hợp lệ."); break; }
            f.dtMax = (int)v;
        }
        if (f.giaMin >= 0 && f.giaMax >= 0 && f.giaMin > f.giaMax) {
            warn(m_hwnd, L"Giá \"từ\" phải nhỏ hơn hoặc bằng giá \"đến\".");
            break;
        }
        if (f.dtMin >= 0 && f.dtMax >= 0 && f.dtMin > f.dtMax) {
            warn(m_hwnd, L"Diện tích \"từ\" phải nhỏ hơn hoặc bằng diện tích \"đến\".");
            break;
        }
        f.chiPhongTrong = isChecked(ctl(IDF_CHECK));
        m_roomFilter = f;
        m_roomFiltered = true;
        m_roomSorted = false;
        setDetail(L"");
        refreshRooms();
        break;
    }
    case IDB_RESET:
        m_roomFiltered = false;
        m_roomFilter = RoomFilter();
        for (int i = IDF_E1; i <= IDF_E4; ++i) setText(ctl(i), L"");
        SendMessageW(ctl(IDF_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
        setDetail(L"");
        refreshRooms();
        break;
    }
}

// =====================================================================
//                        Màn hình Khách hàng
// =====================================================================

void WinApp::buildCustomers() {
    setText(m_title, L"Quản lý khách hàng");
    tbButton(0, IDB_ADD, L"Thêm khách hàng", 120);
    tbButton(0, IDB_EDIT, L"Sửa", 64);
    tbButton(0, IDB_DELETE, L"Xóa", 64);
    tbLabel(1, L"Tra cứu (Mã KH / CCCD / SĐT / tên):", 230);
    tbEdit(1, IDF_E1, 240);
    tbButton(1, IDB_SEARCH, L"Tìm", 70);
    tbButton(1, IDB_RESET, L"Bỏ lọc", 72);
    m_defBtn = IDB_SEARCH;
    lvSetColumns(m_list, {{L"Mã KH", 70, false},
                          {L"Tên khách hàng", 190, false},
                          {L"Địa chỉ", 280, false},
                          {L"CCCD", 130, false},
                          {L"Số điện thoại", 120, false},
                          {L"Tài khoản", 110, false},
                          {L"Phòng đang thuê", 120, false}});
}

void WinApp::refreshCustomers() {
    DynArray<KhachHang*> rows;
    std::wstring q = trim(getText(ctl(IDF_E1)));
    std::wstring how;
    if (q.empty()) {
        m_db.tenants.getCustomers(rows);
        Sorter::quickSort(rows, [](KhachHang* a, KhachHang* b) { return a->maKhachHang < b->maKhachHang; });
    } else {
        std::string key = U8(q);
        KhachHang* kh = m_db.tenants.findByCCCD(key);
        how = L"CCCD";
        if (!kh) {
            kh = m_db.tenants.findBySDT(key);
            how = L"SĐT";
        }
        if (!kh && isDigits(key) && key.size() <= 6) {
            kh = m_db.tenants.findCustomer(std::stoi(key));
            how = L"Mã KH";
        }
        if (kh) {
            rows.push(kh);
        } else {
            // Không khớp khóa băm: tìm tuần tự theo tên
            how = L"tên (tìm tuần tự)";
            DynArray<KhachHang*> all;
            m_db.tenants.getCustomers(all);
            std::wstring lq = lower(q);
            for (int i = 0; i < all.size(); ++i)
                if (lower(W(all[i]->tenKhachHang)).find(lq) != std::wstring::npos) rows.push(all[i]);
            Sorter::quickSort(rows, [](KhachHang* a, KhachHang* b) { return a->maKhachHang < b->maKhachHang; });
        }
    }

    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const KhachHang* k = rows[i];
        TaiKhoan* tk = m_db.tenants.findAccount(k->maTaiKhoan);
        PhieuThueTraPhong* p = m_db.activeRentalOfCustomer(k->cccd);
        lvAddRow(m_list,
                 {num(k->maKhachHang), W(k->tenKhachHang), W(k->diaChi), W(k->cccd), W(k->sdt),
                  tk ? W(tk->userName) : L"", p ? num(p->maPhong) : L""},
                 k->maKhachHang);
    }
    lvEndUpdate(m_list);

    if (q.empty()) setStatus(L"Tổng " + num(m_db.tenants.customerCount()) + L" khách hàng");
    else setStatus(L"Tìm theo " + how + L": " + num(rows.size()) + L" kết quả");
    setDetail(L"Bảng băm Mã KH: " + num(m_db.tenants.customerCount()) + L" phần tử / " +
              num(m_db.tenants.customerBuckets()) + L" bucket, chuỗi dài nhất = " +
              num(m_db.tenants.customerLongestChain()));
}

void WinApp::customerCommand(int id) {
    int sel = (int)lvSelectedData(m_list);
    switch (id) {
    case IDB_ADD: customerDialog(0); break;
    case IDB_EDIT:
        if (sel > 0) customerDialog(sel);
        else warn(m_hwnd, L"Hãy chọn khách hàng cần sửa.");
        break;
    case IDB_DELETE: {
        if (sel <= 0) {
            warn(m_hwnd, L"Hãy chọn khách hàng cần xóa.");
            break;
        }
        KhachHang* kh = m_db.tenants.findCustomer(sel);
        if (!kh || !confirm(m_hwnd, L"Xóa khách hàng \"" + W(kh->tenKhachHang) + L"\" (CCCD " + W(kh->cccd) +
                                        L")?\nTài khoản đăng nhập của khách (nếu có) cũng bị xóa."))
            break;
        std::string err = m_db.deleteCustomer(sel);
        if (!err.empty()) {
            warn(m_hwnd, W(err));
            break;
        }
        commit();
        refreshCustomers();
        break;
    }
    case IDB_SEARCH: refreshCustomers(); break;
    case IDB_RESET:
        setText(ctl(IDF_E1), L"");
        refreshCustomers();
        break;
    }
}

// =====================================================================
//                     Màn hình Phiếu thuê / trả phòng
// =====================================================================

void WinApp::buildRentals() {
    setText(m_title, L"Phiếu thuê / trả phòng");
    tbButton(0, IDB_RENT, L"Lập phiếu thuê...", 124);
    tbButton(0, IDB_RETURN, L"Lập phiếu trả...", 120);
    tbButton(0, IDB_BILL, L"Lập phiếu thu...", 118);
    tbButton(0, IDB_DELETE, L"Xóa phiếu", 84);
    tbGap(0, 14);
    tbLabel(0, L"Hiển thị:", 58);
    tbCombo(0, IDF_C1, 130, {L"Tất cả", L"Đang thuê", L"Đã trả phòng"}, 0);
    lvSetColumns(m_list, {{L"Mã phiếu", 70, false},
                          {L"Phòng", 60, false},
                          {L"Khách hàng", 170, false},
                          {L"CCCD", 120, false},
                          {L"SĐT", 105, false},
                          {L"Địa chỉ", 210, false},
                          {L"Ngày thuê", 90, false},
                          {L"Ngày trả", 90, false},
                          {L"Số tháng", 70, true},
                          {L"Quản lý", 130, false},
                          {L"Trạng thái", 90, false}});
}

void WinApp::refreshRentals() {
    int mode = comboSel(ctl(IDF_C1));
    Date today = Date::today();
    int active = 0, shown = 0;
    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < m_db.rentals.size(); ++i) {
        const PhieuThueTraPhong* p = m_db.rentals[i];
        if (p->dangThue()) ++active;
        if ((mode == 1 && !p->dangThue()) || (mode == 2 && p->dangThue())) continue;
        ++shown;
        lvAddRow(m_list,
                 {num(p->maPhieuTTP), num(p->maPhong), W(p->tenKhachHang), W(p->cccd), W(p->sdt), W(p->diaChi),
                  W(p->ngayThue.toDisplay()), W(p->ngayTra.toDisplay()), num(BillingService::soThangThue(*p, today)),
                  managerName(p->maQuanLy), p->dangThue() ? L"Đang thuê" : L"Đã trả"},
                 p->maPhieuTTP);
    }
    lvEndUpdate(m_list);
    setStatus(L"Hiển thị " + num(shown) + L" phiếu  |  Đang thuê: " + num(active) + L"  |  Đã trả: " +
              num(m_db.rentals.size() - active));
    if (m_db.rentals.size() == 0) setDetail(L"Chưa có phiếu thuê nào được lập.");
}

void WinApp::rentalCommand(int id) {
    int sel = (int)lvSelectedData(m_list);
    PhieuThueTraPhong* p = sel > 0 ? m_db.findRental(sel) : nullptr;
    switch (id) {
    case IDB_RENT: rentDialog(0); break;
    case IDB_RETURN: returnDialog(p && p->dangThue() ? p->maPhieuTTP : 0); break;
    case IDB_BILL: billDialog(p ? p->maPhieuTTP : 0); break;
    case IDB_DELETE: {
        if (!p) {
            warn(m_hwnd, L"Hãy chọn phiếu cần xóa.");
            break;
        }
        std::wstring msg = L"Xóa phiếu thuê #" + num(p->maPhieuTTP) + L" (phòng " + num(p->maPhong) + L")?";
        if (p->dangThue()) msg += L"\nPhòng sẽ được chuyển về trạng thái Trống.";
        if (!confirm(m_hwnd, msg)) break;
        std::string err = m_db.deleteRental(sel);
        if (!err.empty()) {
            warn(m_hwnd, W(err));
            break;
        }
        commit();
        refreshRentals();
        break;
    }
    }
}

// =====================================================================
//                         Màn hình Phiếu thu tiền
// =====================================================================

void WinApp::buildBills() {
    setText(m_title, L"Phiếu thu tiền");
    tbButton(0, IDB_BILL, L"Lập phiếu thu...", 118);
    tbButton(0, IDB_PAY, L"Thanh toán", 90);
    tbButton(0, IDB_DETAIL, L"Chi tiết", 76);
    tbButton(0, IDB_DELETE, L"Xóa", 64);
    tbGap(0, 14);
    tbLabel(0, L"Sắp xếp:", 58);
    tbCombo(0, IDF_C2, 110, {L"Tháng", L"Tổng tiền", L"Mã phiếu"}, m_billSortKey);
    tbCombo(0, IDF_C3, 96, {L"Tăng dần", L"Giảm dần"}, m_billSortAsc ? 0 : 1);
    tbButton(0, IDB_SORT, L"Sắp xếp", 80);

    tbLabel(1, L"Tháng", 40);
    tbEdit(1, IDF_E1, 44);
    tbLabel(1, L"Năm", 32);
    tbEdit(1, IDF_E2, 60);
    tbLabel(1, L"Trạng thái", 64);
    tbCombo(1, IDF_C1, 150, {L"Tất cả", L"Chưa thanh toán", L"Đã thanh toán"}, 0);
    tbButton(1, IDB_SEARCH, L"Lọc", 70);
    tbButton(1, IDB_RESET, L"Bỏ lọc", 72);
    m_defBtn = IDB_SEARCH;

    lvSetColumns(m_list, {{L"Mã phiếu", 70, false},
                          {L"Phòng", 56, false},
                          {L"Khách hàng", 150, false},
                          {L"Kỳ", 70, false},
                          {L"Tiền phòng", 92, true},
                          {L"Điện (kWh)", 74, true},
                          {L"Tiền điện", 84, true},
                          {L"Nước (m³)", 72, true},
                          {L"Tiền nước", 80, true},
                          {L"Wifi", 72, true},
                          {L"Tổng tiền", 100, true},
                          {L"Hạn TT", 86, false},
                          {L"Trạng thái", 110, false}});
}

void WinApp::refreshBills() {
    long long thang = 0, nam = 0;
    parseNumber(getText(ctl(IDF_E1)), thang);
    parseNumber(getText(ctl(IDF_E2)), nam);
    int mode = comboSel(ctl(IDF_C1));
    m_billSortKey = comboSel(ctl(IDF_C2));
    m_billSortAsc = comboSel(ctl(IDF_C3)) == 0;

    DynArray<PhieuThuTien*> rows;
    long long tong = 0, daThu = 0;
    for (int i = 0; i < m_db.bills.size(); ++i) {
        PhieuThuTien* b = m_db.bills[i];
        if (thang > 0 && b->thang != thang) continue;
        if (nam > 0 && b->nam != nam) continue;
        if ((mode == 1 && b->daThanhToan) || (mode == 2 && !b->daThanhToan)) continue;
        rows.push(b);
        tong += b->tongTien;
        if (b->daThanhToan) daThu += b->tongTien;
    }
    Sorter::quickSortBills(rows, (BillSortKey)m_billSortKey, m_billSortAsc);

    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const PhieuThuTien* b = rows[i];
        lvAddRow(m_list,
                 {num(b->maPhieuThuTien), num(b->maPhong), customerName(b->maKhachHang), kyThu(b->thang, b->nam),
                  money(b->tienPhong), num(b->soDien), money(b->tienDien), num(b->soNuoc), money(b->tienNuoc),
                  money(b->tienWifi), money(b->tongTien), W(b->hanThanhToan.toDisplay()),
                  b->daThanhToan ? L"Đã thanh toán" : L"Chưa thanh toán"},
                 b->maPhieuThuTien);
    }
    lvEndUpdate(m_list);
    setStatus(num(rows.size()) + L" phiếu  |  Tổng: " + money(tong) + L"đ  |  Đã thu: " + money(daThu) +
              L"đ  |  Còn nợ: " + money(tong - daThu) + L"đ");
}

void WinApp::billCommand(int id) {
    int sel = (int)lvSelectedData(m_list);
    switch (id) {
    case IDB_BILL: billDialog(0); break;
    case IDB_PAY:
        if (sel > 0) payBill(sel);
        else warn(m_hwnd, L"Hãy chọn phiếu thu cần thanh toán.");
        break;
    case IDB_DETAIL:
        if (sel > 0) billDetail(sel);
        else warn(m_hwnd, L"Hãy chọn một phiếu thu.");
        break;
    case IDB_DELETE: {
        if (sel <= 0) {
            warn(m_hwnd, L"Hãy chọn phiếu thu cần xóa.");
            break;
        }
        if (!confirm(m_hwnd, L"Xóa phiếu thu #" + num(sel) + L"?")) break;
        std::string err = m_db.deleteBill(sel);
        if (!err.empty()) {
            warn(m_hwnd, W(err));
            break;
        }
        commit();
        refresh();
        break;
    }
    case IDB_SORT:
    case IDB_SEARCH: refreshBills(); break;
    case IDB_RESET:
        setText(ctl(IDF_E1), L"");
        setText(ctl(IDF_E2), L"");
        SendMessageW(ctl(IDF_C1), CB_SETCURSEL, 0, 0);
        refreshBills();
        break;
    }
}

// =====================================================================
//                      Thống kê: Chưa thanh toán
// =====================================================================

void WinApp::buildUnpaid() {
    setText(m_title, L"Thống kê phòng chưa thanh toán");
    tbButton(0, IDB_PAY, L"Thanh toán", 90);
    tbButton(0, IDB_DETAIL, L"Chi tiết", 76);
    tbGap(0, 14);
    tbLabel(0, L"Tháng", 40);
    tbEdit(0, IDF_E1, 44);
    tbLabel(0, L"Năm", 32);
    tbEdit(0, IDF_E2, 60);
    tbButton(0, IDB_SEARCH, L"Xem", 70);
    tbLabel(0, L"(để trống = mọi tháng)", 150);
    m_defBtn = IDB_SEARCH;
    lvSetColumns(m_list, {{L"Ưu tiên", 60, false},
                          {L"Mã phiếu", 70, false},
                          {L"Phòng", 56, false},
                          {L"Khách hàng", 170, false},
                          {L"SĐT", 105, false},
                          {L"Kỳ", 70, false},
                          {L"Số tiền còn thiếu", 120, true},
                          {L"Hạn thanh toán", 110, false},
                          {L"Quá hạn (ngày)", 110, true},
                          {L"Mức độ", 120, false},
                          {L"Quản lý lập", 130, false}});
}

void WinApp::refreshUnpaid() {
    long long thang = 0, nam = 0;
    std::wstring tThang = trim(getText(ctl(IDF_E1))), tNam = trim(getText(ctl(IDF_E2)));
    if (!tThang.empty() && (!parseNumber(tThang, thang) || thang < 1 || thang > 12)) {
        warn(m_hwnd, L"Tháng phải từ 1 đến 12.");
        return;
    }
    if (!tThang.empty()) {
        if (tNam.empty()) nam = Date::today().y;
        else if (!parseNumber(tNam, nam)) {
            warn(m_hwnd, L"Năm không hợp lệ.");
            return;
        }
    }

    DynArray<PhieuThuTien*> rows;
    long long tong = BillingService::chuaThanhToan(m_db, (int)thang, (int)nam, rows);
    Date today = Date::today();

    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const PhieuThuTien* b = rows[i];
        KhachHang* kh = m_db.tenants.findCustomer(b->maKhachHang);
        int qh = BillingService::soNgayQuaHan(*b, today);
        lvAddRow(m_list,
                 {num(i + 1), num(b->maPhieuThuTien), num(b->maPhong), kh ? W(kh->tenKhachHang) : L"",
                  kh ? W(kh->sdt) : L"", kyThu(b->thang, b->nam), money(b->tongTien), W(b->hanThanhToan.toDisplay()),
                  qh > 0 ? num(qh) : L"0", W(BillingService::mucDoKhanCap(qh)), managerName(b->maQuanLy)},
                 b->maPhieuThuTien);
    }
    lvEndUpdate(m_list);

    if (rows.size() == 0) {
        setStatus(L"Không có phòng nào còn thiếu tiền" + (thang > 0 ? L" trong tháng " + kyThu((int)thang, (int)nam)
                                                                     : std::wstring(L".")));
    } else {
        setStatus(num(rows.size()) + L" phiếu chưa thanh toán  |  Tổng nợ: " + money(tong) + L"đ");
    }
    PhieuThuTien* top = m_db.unpaid.top();
    setDetail(top ? L"Min-Heap: " + num(m_db.unpaid.size()) + L" phiếu, đỉnh heap = phiếu #" +
                        num(top->maPhieuThuTien) + L" (hạn " + W(top->hanThanhToan.toDisplay()) + L")"
                  : L"Min-Heap rỗng");
}

// =====================================================================
//                         Thống kê: Doanh thu
// =====================================================================

void WinApp::buildRevenue() {
    setText(m_title, L"Thống kê doanh thu");
    Date today = Date::today();
    tbLabel(0, L"Năm", 32);
    tbEdit(0, IDF_E1, 64, num(today.y));
    tbLabel(0, L"Tháng", 40);
    std::vector<std::wstring> months = {L"Cả năm"};
    for (int m = 1; m <= 12; ++m) months.push_back(L"Tháng " + num(m));
    tbCombo(0, IDF_C1, 110, months, 0);
    tbButton(0, IDB_SEARCH, L"Xem", 70);
    tbLabel(0, L"Chỉ tính các phiếu thu đã thanh toán.", 260);
    m_defBtn = IDB_SEARCH;
}

void WinApp::refreshRevenue() {
    long long nam;
    if (!parseNumber(getText(ctl(IDF_E1)), nam) || nam < 2000 || nam > 2100) {
        warn(m_hwnd, L"Vui lòng nhập năm hợp lệ (2000 - 2100).");
        return;
    }
    int thang = comboSel(ctl(IDF_C1));

    lvBeginUpdate(m_list);
    if (thang <= 0) {
        lvSetColumns(m_list, {{L"Tháng", 90, false},
                              {L"Số phiếu đã thu", 120, true},
                              {L"Tiền phòng", 130, true},
                              {L"Tiền điện", 120, true},
                              {L"Tiền nước", 120, true},
                              {L"Tiền wifi", 110, true},
                              {L"Doanh thu", 150, true}});
        RevenueRow rows[12];
        long long tong = BillingService::doanhThuNam(m_db, (int)nam, rows);
        RevenueRow sum;
        for (int i = 0; i < 12; ++i) {
            const RevenueRow& r = rows[i];
            lvAddRow(m_list,
                     {kyThu(r.thang, (int)nam), num(r.soPhieu), money(r.tienPhong), money(r.tienDien),
                      money(r.tienNuoc), money(r.tienWifi), money(r.tong)},
                     r.thang);
            sum.soPhieu += r.soPhieu;
            sum.tienPhong += r.tienPhong;
            sum.tienDien += r.tienDien;
            sum.tienNuoc += r.tienNuoc;
            sum.tienWifi += r.tienWifi;
        }
        lvAddRow(m_list,
                 {L"TỔNG NĂM", num(sum.soPhieu), money(sum.tienPhong), money(sum.tienDien), money(sum.tienNuoc),
                  money(sum.tienWifi), money(tong)},
                 0);
        setStatus(tong > 0 ? L"Tổng doanh thu năm " + num(nam) + L": " + money(tong) + L"đ"
                           : L"Không có dữ liệu doanh thu trong năm " + num(nam) + L".");
    } else {
        lvSetColumns(m_list, {{L"Mã phiếu", 80, false},
                              {L"Phòng", 60, false},
                              {L"Khách hàng", 180, false},
                              {L"Tiền phòng", 110, true},
                              {L"Tiền điện", 100, true},
                              {L"Tiền nước", 100, true},
                              {L"Tiền wifi", 90, true},
                              {L"Tổng tiền", 120, true},
                              {L"Quản lý lập", 140, false}});
        DynArray<PhieuThuTien*> rows;
        long long tong = BillingService::doanhThuThang(m_db, thang, (int)nam, rows);
        for (int i = 0; i < rows.size(); ++i) {
            const PhieuThuTien* b = rows[i];
            lvAddRow(m_list,
                     {num(b->maPhieuThuTien), num(b->maPhong), customerName(b->maKhachHang), money(b->tienPhong),
                      money(b->tienDien), money(b->tienNuoc), money(b->tienWifi), money(b->tongTien),
                      managerName(b->maQuanLy)},
                     b->maPhieuThuTien);
        }
        setStatus(rows.size() > 0 ? L"Doanh thu tháng " + kyThu(thang, (int)nam) + L": " + money(tong) + L"đ (" +
                                        num(rows.size()) + L" phiếu)"
                                  : L"Không có dữ liệu doanh thu trong tháng " + kyThu(thang, (int)nam) + L".");
    }
    lvEndUpdate(m_list);
}

// =====================================================================
//                   Thống kê: Khách hàng thân thiết
// =====================================================================

void WinApp::buildLoyal() {
    setText(m_title, L"Khách hàng thân thiết");
    tbLabel(0, L"Tiêu chí:", 58);
    tbCombo(0, IDF_C1, 190, {L"Số tháng đã thuê", L"Số lần thuê phòng", L"Tổng tiền đã thanh toán"}, 0);
    tbLabel(0, L"Tối thiểu", 60);
    tbEdit(0, IDF_E1, 100, L"3");
    tbButton(0, IDB_SEARCH, L"Xem", 70);
    m_defBtn = IDB_SEARCH;
    lvSetColumns(m_list, {{L"Hạng", 56, false},
                          {L"Mã KH", 70, false},
                          {L"Tên khách hàng", 200, false},
                          {L"Số điện thoại", 120, false},
                          {L"Số lần thuê", 100, true},
                          {L"Số tháng đã thuê", 130, true},
                          {L"Tổng đã thanh toán", 150, true},
                          {L"Đang thuê phòng", 120, false}});
}

void WinApp::refreshLoyal() {
    long long toiThieu = 0;
    std::wstring t = trim(getText(ctl(IDF_E1)));
    if (!t.empty() && !parseNumber(t, toiThieu)) {
        warn(m_hwnd, L"Giá trị tối thiểu không hợp lệ.");
        return;
    }
    LoyalCriteria c = (LoyalCriteria)comboSel(ctl(IDF_C1));
    DynArray<LoyalCustomer> rows;
    BillingService::khachHangThanThiet(m_db, c, toiThieu, rows);

    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const LoyalCustomer& r = rows[i];
        PhieuThueTraPhong* p = m_db.activeRentalOfCustomer(r.kh->cccd);
        lvAddRow(m_list,
                 {num(i + 1), num(r.kh->maKhachHang), W(r.kh->tenKhachHang), W(r.kh->sdt), num(r.soLanThue),
                  num(r.soThangThue), money(r.tongDaThanhToan), p ? num(p->maPhong) : L""},
                 r.kh->maKhachHang);
    }
    lvEndUpdate(m_list);
    setStatus(rows.size() > 0 ? num(rows.size()) + L" khách hàng đạt tiêu chí (sắp giảm dần bằng QuickSort)"
                              : L"Không có khách hàng nào đáp ứng tiêu chí đã chọn.");
}

// =====================================================================
//                         Màn hình Tài khoản
// =====================================================================

void WinApp::buildAccounts() {
    setText(m_title, L"Quản lý tài khoản");
    tbButton(0, IDB_ADD, L"Thêm TK quản lý...", 136);
    tbButton(0, IDB_PASS, L"Đặt lại mật khẩu...", 136);
    tbButton(0, IDB_DELETE, L"Xóa", 64);
    lvSetColumns(m_list, {{L"Mã TK", 70, false},
                          {L"Tên đăng nhập", 160, false},
                          {L"Vai trò", 110, false},
                          {L"Chủ tài khoản", 220, false},
                          {L"Mã KH / Mã QL", 110, false}});
}

void WinApp::refreshAccounts() {
    DynArray<TaiKhoan*> rows;
    m_db.tenants.getAccounts(rows);
    Sorter::quickSort(rows, [](TaiKhoan* a, TaiKhoan* b) { return a->maTaiKhoan < b->maTaiKhoan; });
    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const TaiKhoan* t = rows[i];
        std::wstring owner, ownerId;
        if (t->role == ROLE_QUAN_LY) {
            QuanLy* ql = m_db.findManagerByAccount(t->maTaiKhoan);
            if (ql) {
                owner = W(ql->tenQuanLy);
                ownerId = L"QL " + num(ql->maQuanLy);
            }
        } else {
            KhachHang* kh = m_db.tenants.findByAccount(t->maTaiKhoan);
            if (kh) {
                owner = W(kh->tenKhachHang);
                ownerId = L"KH " + num(kh->maKhachHang);
            }
        }
        lvAddRow(m_list, {num(t->maTaiKhoan), W(t->userName), W(tenVaiTro(t->role)), owner, ownerId}, t->maTaiKhoan);
    }
    lvEndUpdate(m_list);
    setStatus(L"Tổng " + num(rows.size()) + L" tài khoản (tra cứu đăng nhập bằng bảng băm theo UserName)");
}

void WinApp::accountCommand(int id) {
    int sel = (int)lvSelectedData(m_list);
    switch (id) {
    case IDB_ADD: managerAccountDialog(); break;
    case IDB_PASS:
        if (sel > 0) changePasswordDialog(sel, sel == m_userId);
        else warn(m_hwnd, L"Hãy chọn tài khoản.");
        break;
    case IDB_DELETE: {
        if (sel <= 0) {
            warn(m_hwnd, L"Hãy chọn tài khoản cần xóa.");
            break;
        }
        TaiKhoan* tk = m_db.tenants.findAccount(sel);
        if (!tk || !confirm(m_hwnd, L"Xóa tài khoản \"" + W(tk->userName) + L"\"?")) break;
        std::string err = m_db.deleteAccount(sel, m_userId);
        if (!err.empty()) {
            warn(m_hwnd, W(err));
            break;
        }
        commit();
        refreshAccounts();
        break;
    }
    }
}

// =====================================================================
//                   Màn hình của Khách hàng (Của tôi)
// =====================================================================

void WinApp::buildMy() {
    setText(m_title, L"Phòng & hóa đơn của tôi");
    tbButton(0, IDB_DETAIL, L"Xem chi tiết", 100);
    tbButton(0, IDB_INFO, L"Cập nhật thông tin...", 146);
    tbButton(0, IDB_PASS, L"Đổi mật khẩu...", 120);
    HWND h = label(m_hwnd, L"", m_x[1], ROW1_Y + 6, 1000, 20);
    SetWindowLongPtrW(h, GWLP_ID, IDF_INFO);
    track(h);
    m_x[1] += 1000;
    lvSetColumns(m_list, {{L"Mã phiếu", 70, false},
                          {L"Phòng", 60, false},
                          {L"Kỳ", 80, false},
                          {L"Tiền phòng", 100, true},
                          {L"Tiền điện", 100, true},
                          {L"Tiền nước", 100, true},
                          {L"Wifi", 80, true},
                          {L"Tổng tiền", 120, true},
                          {L"Hạn thanh toán", 110, false},
                          {L"Trạng thái", 120, false}});
}

void WinApp::refreshMy() {
    KhachHang* kh = m_db.tenants.findByAccount(m_userId);
    if (!kh) {
        setText(ctl(IDF_INFO), L"Tài khoản chưa liên kết hồ sơ khách hàng.");
        return;
    }
    PhieuThueTraPhong* p = m_db.activeRentalOfCustomer(kh->cccd);
    std::wstring s = W(kh->tenKhachHang) + L"  |  CCCD: " + W(kh->cccd) + L"  |  SĐT: " + W(kh->sdt) + L"  |  ";
    if (p) {
        RoomNode* r = m_db.rooms.find(p->maPhong);
        s += L"Đang thuê phòng " + num(p->maPhong) + L" từ " + W(p->ngayThue.toDisplay());
        if (r) s += L" (" + money(r->data.giaThue) + L"đ/tháng, " + num(r->data.dienTich) + L"m²)";
    } else {
        s += L"Hiện không thuê phòng nào";
    }
    setText(ctl(IDF_INFO), s);

    DynArray<PhieuThuTien*> rows;
    long long no = 0;
    for (int i = 0; i < m_db.bills.size(); ++i)
        if (m_db.bills[i]->maKhachHang == kh->maKhachHang) {
            rows.push(m_db.bills[i]);
            if (!m_db.bills[i]->daThanhToan) no += m_db.bills[i]->tongTien;
        }
    Sorter::quickSortBills(rows, BILL_BY_THANG, false);

    lvBeginUpdate(m_list);
    lvClear(m_list);
    for (int i = 0; i < rows.size(); ++i) {
        const PhieuThuTien* b = rows[i];
        lvAddRow(m_list,
                 {num(b->maPhieuThuTien), num(b->maPhong), kyThu(b->thang, b->nam), money(b->tienPhong),
                  money(b->tienDien), money(b->tienNuoc), money(b->tienWifi), money(b->tongTien),
                  W(b->hanThanhToan.toDisplay()), b->daThanhToan ? L"Đã thanh toán" : L"Chưa thanh toán"},
                 b->maPhieuThuTien);
    }
    lvEndUpdate(m_list);
    setStatus(num(rows.size()) + L" phiếu thu  |  Còn nợ: " + money(no) + L"đ");
}

// =====================================================================
//                          Hộp thoại nghiệp vụ
// =====================================================================

void WinApp::roomDialog(int maPhong) {
    bool editing = maPhong > 0;
    PhongTro cur;
    if (editing) {
        RoomNode* n = m_db.rooms.find(maPhong);
        if (!n) return;
        cur = n->data;
    }

    FormDialog f(editing ? L"Sửa thông tin phòng trọ" : L"Thêm phòng trọ", 520);
    int iMa = f.add(L"Mã phòng", editing ? FT_READONLY : FT_NUMBER, editing ? num(cur.maPhong) : L"");
    int iGia = f.add(L"Giá thuê (đ/tháng)", FT_NUMBER, editing ? money(cur.giaThue) : L"");
    int iDt = f.add(L"Diện tích (m²)", FT_NUMBER, editing ? num(cur.dienTich) : L"");
    int iVc = f.add(L"Vật chất", FT_TEXT, W(cur.vatChat));
    int iTt = f.addCombo(L"Trạng thái", {L"Trống", L"Đã thuê"}, cur.trangThai);
    int iHinh = f.add(L"Hình (đường dẫn ảnh)", FT_TEXT, W(cur.hinh));
    f.addButton(ID_BROWSE, L"Chọn ảnh...");
    f.setNote(L"QD01: Chỉ chủ trọ được thay đổi thông tin phòng. Mã phòng không được trùng và không thể sửa sau khi tạo.");
    f.setOkText(editing ? L"Lưu" : L"Thêm phòng");

    int savedMa = 0;
    f.onSubmit = [&](FormDialog& d, int id) -> std::wstring {
        if (id == ID_BROWSE) {
            wchar_t file[MAX_PATH] = L"";
            OPENFILENAMEW ofn = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = d.hwnd();
            ofn.lpstrFilter = L"Ảnh (*.jpg;*.png;*.bmp)\0*.jpg;*.jpeg;*.png;*.bmp\0Tất cả\0*.*\0";
            ofn.lpstrFile = file;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetOpenFileNameW(&ofn)) d.setText(iHinh, file);
            return FormDialog::STAY;
        }
        PhongTro p = cur;
        long long v;
        if (!editing) {
            if (!d.number(iMa, v) || v <= 0 || v > 999999) return L"Mã phòng phải là số nguyên dương.";
            p.maPhong = (int)v;
        }
        if (!d.number(iGia, v)) return L"Giá thuê không hợp lệ.";
        p.giaThue = v;
        if (!d.number(iDt, v) || v > 100000) return L"Diện tích không hợp lệ.";
        p.dienTich = (int)v;
        p.vatChat = U8(trim(d.text(iVc)));
        p.trangThai = d.sel(iTt);
        p.hinh = U8(trim(d.text(iHinh)));
        std::string err = editing ? m_db.updateRoom(p) : m_db.addRoom(p);
        if (!err.empty()) return W(err);
        savedMa = p.maPhong;
        return L"";
    };

    if (f.show(m_hwnd) == IDOK) {
        commit();
        m_curRoom = savedMa;
        if (m_view == V_ROOMS) refreshRooms();
        setDetail(editing ? L"Đã cập nhật phòng " + num(savedMa) + L"." : L"Đã thêm phòng " + num(savedMa) + L".");
    }
}

void WinApp::importRooms() {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"File CSV (*.csv)\0*.csv\0Tất cả\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Chọn file CSV danh sách phòng (MaPhong,GiaThue,DienTich,VatChat,TrangThai,Hinh)";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return;

    int skipped = 0;
    std::string err;
    int added = m_storage.importRooms(file, m_db, skipped, err);
    if (added < 0) {
        warn(m_hwnd, W(err));
        return;
    }
    commit();
    if (m_view == V_ROOMS) refreshRooms();
    info(m_hwnd, L"Đã import " + num(added) + L" phòng mới.\nBỏ qua " + num(skipped) +
                     L" dòng (trùng mã phòng hoặc dữ liệu không hợp lệ).");
}

void WinApp::customerDialog(int maKH) {
    bool editing = maKH > 0;
    KhachHang cur;
    if (editing) {
        KhachHang* k = m_db.tenants.findCustomer(maKH);
        if (!k) return;
        cur = *k;
    }
    FormDialog f(editing ? L"Sửa thông tin khách hàng" : L"Thêm khách hàng", 520);
    if (editing) f.add(L"Mã khách hàng", FT_READONLY, num(cur.maKhachHang));
    int iTen = f.add(L"Tên khách hàng", FT_TEXT, W(cur.tenKhachHang));
    int iDc = f.add(L"Địa chỉ", FT_TEXT, W(cur.diaChi));
    int iCccd = f.add(L"CCCD", FT_TEXT, W(cur.cccd));
    int iSdt = f.add(L"Số điện thoại", FT_TEXT, W(cur.sdt));
    f.setNote(isManager() ? L"QD02: Chỉ thay đổi thông tin khách hàng khi có yêu cầu của khách hàng."
                          : L"Thông tin này dùng để lập phiếu thuê và phiếu thu tiền.");
    f.setOkText(editing ? L"Lưu" : L"Thêm");
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        KhachHang kh = cur;
        kh.tenKhachHang = U8(trim(d.text(iTen)));
        kh.diaChi = U8(trim(d.text(iDc)));
        kh.cccd = U8(trim(d.text(iCccd)));
        kh.sdt = U8(trim(d.text(iSdt)));
        return W(editing ? m_db.updateCustomer(kh) : m_db.addCustomer(kh));
    };
    if (f.show(m_hwnd) == IDOK) {
        commit();
        refresh();
        setDetail(editing ? L"Đã cập nhật thông tin khách hàng." : L"Đã thêm khách hàng mới.");
    }
}

void WinApp::rentDialog(int maPhong) {
    std::vector<int> roomIds;
    std::vector<std::wstring> items;
    int sel = 0;
    for (RoomNode* n = m_db.rooms.head(); n; n = n->next) {
        if (n->data.trangThai != PHONG_TRONG) continue;
        if (n->data.maPhong == maPhong) sel = (int)roomIds.size();
        roomIds.push_back(n->data.maPhong);
        items.push_back(L"Phòng " + num(n->data.maPhong) + L"  —  " + money(n->data.giaThue) + L"đ  —  " +
                        num(n->data.dienTich) + L"m²");
    }
    if (roomIds.empty()) {
        info(m_hwnd, L"Hiện không còn phòng trống để cho thuê.");
        return;
    }
    if (maPhong > 0 && (roomIds[sel] != maPhong)) {
        warn(m_hwnd, L"Phòng " + num(maPhong) + L" không còn trống. Hãy chọn phòng khác trong danh sách.");
    }

    FormDialog f(L"Lập phiếu thuê phòng", 540);
    f.add(L"Mã phiếu thuê", FT_READONLY, num(m_db.nextMaPhieuTTP()));
    int iPh = f.addCombo(L"Phòng (đang trống)", items, sel);
    int iGia = f.add(L"Giá thuê (đ/tháng)", FT_READONLY);
    int iCoc = f.add(L"Tiền cọc 50% (QD04)", FT_READONLY);
    int iCccd = f.add(L"CCCD khách hàng", FT_TEXT);
    int iTen = f.add(L"Tên khách hàng", FT_TEXT);
    int iSdt = f.add(L"Số điện thoại", FT_TEXT);
    int iDc = f.add(L"Địa chỉ", FT_TEXT);
    int iNgay = f.addDate(L"Ngày thuê", Date::today());
    f.add(L"Quản lý lập phiếu", FT_READONLY, managerName(m_maQL));
    f.setNote(L"Nhập CCCD: nếu khách đã có trong hệ thống (tra bảng băm) thông tin sẽ tự điền; "
              L"nếu chưa có, hồ sơ khách hàng mới sẽ được tạo.");
    f.setOkText(L"Tạo phiếu");

    f.onChange = [&](FormDialog& d, int idx) {
        if (idx == -1 || idx == iPh) {
            int s = d.sel(iPh);
            RoomNode* n = s >= 0 ? m_db.rooms.find(roomIds[s]) : nullptr;
            if (n) {
                d.setText(iGia, money(n->data.giaThue));
                d.setText(iCoc, money(BillingService::tienCoc(n->data.giaThue)));
            }
        }
        if (idx == iCccd) {
            KhachHang* k = m_db.tenants.findByCCCD(U8(trim(d.text(iCccd))));
            if (k) {
                d.setText(iTen, W(k->tenKhachHang));
                d.setText(iSdt, W(k->sdt));
                d.setText(iDc, W(k->diaChi));
            }
        }
    };

    int newId = 0, roomId = 0;
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        PhieuThueTraPhong p;
        p.maPhong = roomIds[d.sel(iPh)];
        p.ngayThue = d.date(iNgay);
        p.maQuanLy = m_maQL;
        KhachHang info;
        info.cccd = U8(trim(d.text(iCccd)));
        info.tenKhachHang = U8(trim(d.text(iTen)));
        info.sdt = U8(trim(d.text(iSdt)));
        info.diaChi = U8(trim(d.text(iDc)));
        std::string err = m_db.rentRoom(p, info);
        if (!err.empty()) return W(err);
        newId = p.maPhieuTTP;
        roomId = p.maPhong;
        return L"";
    };

    if (f.show(m_hwnd) == IDOK) {
        commit();
        refresh();
        RoomNode* n = m_db.rooms.find(roomId);
        info(m_hwnd, L"Đã lập phiếu thuê #" + num(newId) + L" cho phòng " + num(roomId) +
                         L".\nPhòng đã chuyển sang trạng thái \"Đã thuê\"." +
                         (n ? L"\nTiền cọc cần thu (QD04): " + money(BillingService::tienCoc(n->data.giaThue)) + L"đ"
                            : L""));
    }
}

void WinApp::returnDialog(int maPhieuTTP) {
    std::vector<int> ids;
    std::vector<std::wstring> items;
    int sel = 0;
    for (int i = 0; i < m_db.rentals.size(); ++i) {
        const PhieuThueTraPhong* p = m_db.rentals[i];
        if (!p->dangThue()) continue;
        if (p->maPhieuTTP == maPhieuTTP) sel = (int)ids.size();
        ids.push_back(p->maPhieuTTP);
        items.push_back(L"#" + num(p->maPhieuTTP) + L"  —  Phòng " + num(p->maPhong) + L"  —  " + W(p->tenKhachHang));
    }
    if (ids.empty()) {
        info(m_hwnd, L"Không có phiếu thuê nào đang hiệu lực.");
        return;
    }

    FormDialog f(L"Lập phiếu trả phòng", 540);
    int iPh = f.addCombo(L"Phiếu thuê", items, sel);
    int iKh = f.add(L"Khách hàng", FT_READONLY);
    int iNgayThue = f.add(L"Ngày thuê", FT_READONLY);
    int iThang = f.add(L"Số tháng đã thuê", FT_READONLY);
    int iNo = f.add(L"Hóa đơn chưa thanh toán", FT_READONLY);
    int iNgayTra = f.addDate(L"Ngày trả phòng", Date::today());
    int iChk = f.addCheck(L"Đã kiểm tra vật chất phòng còn nguyên vẹn (QD05)", false);
    f.setNote(L"QD05: Khách trả phòng phải đảm bảo cơ sở vật chất như lúc thuê, nếu hư hỏng phải đền bù.");
    f.setOkText(L"Xác nhận trả");

    f.onChange = [&](FormDialog& d, int idx) {
        if (idx != -1 && idx != iPh) return;
        PhieuThueTraPhong* p = m_db.findRental(ids[d.sel(iPh)]);
        if (!p) return;
        long long no = 0;
        int c = m_db.unpaidCountOfRental(p->maPhieuTTP, &no);
        d.setText(iKh, W(p->tenKhachHang) + L" — " + W(p->sdt));
        d.setText(iNgayThue, W(p->ngayThue.toDisplay()));
        d.setText(iThang, num(BillingService::soThangThue(*p, Date::today())));
        d.setText(iNo, c > 0 ? num(c) + L" phiếu, tổng " + money(no) + L"đ" : L"Không");
    };

    int doneId = 0;
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        int id = ids[d.sel(iPh)];
        if (!d.checked(iChk)) return L"Vui lòng xác nhận đã kiểm tra vật chất phòng (QD05).";
        long long no = 0;
        int c = m_db.unpaidCountOfRental(id, &no);
        if (c > 0 && !confirm(d.hwnd(), L"Khách hàng còn " + num(c) + L" phiếu chưa thanh toán (" + money(no) +
                                            L"đ).\nVẫn lập phiếu trả phòng?"))
            return FormDialog::STAY;
        std::string err = m_db.returnRoom(id, d.date(iNgayTra));
        if (!err.empty()) return W(err);
        doneId = id;
        return L"";
    };

    if (f.show(m_hwnd) == IDOK) {
        commit();
        refresh();
        PhieuThueTraPhong* p = m_db.findRental(doneId);
        info(m_hwnd, L"Đã trả phòng" + (p ? L" " + num(p->maPhong) : std::wstring()) +
                         L". Phòng đã chuyển về trạng thái \"Trống\".");
    }
}

void WinApp::billDialog(int maPhieuTTP) {
    std::vector<int> ids;
    std::vector<std::wstring> items;
    int sel = 0;
    // Phiếu đang thuê trước, phiếu đã trả sau (để lập phiếu thu tháng cuối)
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < m_db.rentals.size(); ++i) {
            const PhieuThueTraPhong* p = m_db.rentals[i];
            if (p->dangThue() != (pass == 0)) continue;
            if (p->maPhieuTTP == maPhieuTTP) sel = (int)ids.size();
            ids.push_back(p->maPhieuTTP);
            items.push_back(L"#" + num(p->maPhieuTTP) + L"  —  Phòng " + num(p->maPhong) + L"  —  " +
                            W(p->tenKhachHang) + (p->dangThue() ? L"" : L"  (đã trả)"));
        }
    }
    if (ids.empty()) {
        info(m_hwnd, L"Chưa có phiếu thuê phòng nào. Hãy lập phiếu thuê trước.");
        return;
    }

    Date today = Date::today();
    FormDialog f(L"Lập phiếu thu tiền", 540);
    f.add(L"Mã phiếu thu", FT_READONLY, num(m_db.nextMaPhieuThu()));
    int iTtp = f.addCombo(L"Phiếu thuê phòng", items, sel);
    int iKh = f.add(L"Khách hàng", FT_READONLY);
    int iGia = f.add(L"Tiền phòng", FT_READONLY);
    int iThang = f.add(L"Tháng", FT_NUMBER, num(today.m));
    int iNam = f.add(L"Năm", FT_NUMBER, num(today.y));
    int iDien = f.add(L"Số kWh điện sử dụng", FT_NUMBER, L"0");
    int iTienDien = f.add(L"Tiền điện", FT_READONLY);
    int iNuoc = f.add(L"Số m³ nước sử dụng", FT_NUMBER, L"0");
    int iTienNuoc = f.add(L"Tiền nước", FT_READONLY);
    int iWifi = f.add(L"Tiền wifi", FT_NUMBER, L"100.000");
    int iTong = f.add(L"TỔNG TIỀN", FT_READONLY);
    int iHan = f.addDate(L"Hạn thanh toán", today.addDays(7));
    int iTt = f.addCombo(L"Trạng thái", {L"Chưa thanh toán", L"Đã thanh toán"}, 0);
    f.setNote(L"Đơn giá: điện " + money(DON_GIA_DIEN) + L"đ/kWh (TS1), nước " + money(DON_GIA_NUOC) +
              L"đ/m³ (TS2). QD03: mỗi phòng chỉ lập một phiếu thu cho mỗi tháng.");
    f.setOkText(L"Tạo phiếu");

    f.onChange = [&](FormDialog& d, int) {
        PhieuThueTraPhong* p = m_db.findRental(ids[d.sel(iTtp)]);
        RoomNode* r = p ? m_db.rooms.find(p->maPhong) : nullptr;
        long long gia = r ? r->data.giaThue : 0, dien = 0, nuoc = 0, wifi = 0;
        d.number(iDien, dien);
        d.number(iNuoc, nuoc);
        d.number(iWifi, wifi);
        long long td = BillingService::tienDien((int)dien), tn = BillingService::tienNuoc((int)nuoc);
        d.setText(iKh, p ? W(p->tenKhachHang) + L" — CCCD " + W(p->cccd) : L"");
        d.setText(iGia, money(gia));
        d.setText(iTienDien, money(td) + L"  (" + num(dien) + L" × " + money(DON_GIA_DIEN) + L")");
        d.setText(iTienNuoc, money(tn) + L"  (" + num(nuoc) + L" × " + money(DON_GIA_NUOC) + L")");
        d.setText(iTong, money(gia + td + tn + wifi) + L"đ");
    };

    int newId = 0;
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        PhieuThuTien b;
        long long v;
        b.maPhieuTTP = ids[d.sel(iTtp)];
        if (!d.number(iThang, v)) return L"Tháng không hợp lệ.";
        b.thang = (int)v;
        if (!d.number(iNam, v)) return L"Năm không hợp lệ.";
        b.nam = (int)v;
        if (!d.number(iDien, v) || v > 100000) return L"Số kWh điện không hợp lệ.";
        b.soDien = (int)v;
        if (!d.number(iNuoc, v) || v > 10000) return L"Số m³ nước không hợp lệ.";
        b.soNuoc = (int)v;
        if (!d.number(iWifi, v)) return L"Tiền wifi không hợp lệ.";
        b.tienWifi = v;
        b.hanThanhToan = d.date(iHan);
        b.daThanhToan = d.sel(iTt) == 1;
        b.maQuanLy = m_maQL;
        std::string err = m_db.createBill(b);
        if (!err.empty()) return W(err);
        newId = b.maPhieuThuTien;
        return L"";
    };

    if (f.show(m_hwnd) == IDOK) {
        commit();
        refresh();
        billDetail(newId);
    }
}

void WinApp::billDetail(int maPhieu) {
    PhieuThuTien* b = m_db.findBill(maPhieu);
    if (!b) return;
    KhachHang* kh = m_db.tenants.findCustomer(b->maKhachHang);
    int qh = BillingService::soNgayQuaHan(*b, Date::today());
    std::wstring s;
    s += L"PHIẾU THU TIỀN #" + num(b->maPhieuThuTien) + L"\n\n";
    s += L"Phòng: " + num(b->maPhong) + L"        Kỳ: " + kyThu(b->thang, b->nam) + L"\n";
    s += L"Khách hàng: " + (kh ? W(kh->tenKhachHang) + L" (Mã KH " + num(kh->maKhachHang) + L")" : std::wstring()) +
         L"\n";
    s += L"Người lập: " + managerName(b->maQuanLy) + L"\n";
    s += L"──────────────────────────────\n";
    s += L"Tiền phòng:  " + money(b->tienPhong) + L" đ\n";
    s += L"Tiền điện:   " + num(b->soDien) + L" kWh × " + money(DON_GIA_DIEN) + L" = " + money(b->tienDien) + L" đ\n";
    s += L"Tiền nước:   " + num(b->soNuoc) + L" m³ × " + money(DON_GIA_NUOC) + L" = " + money(b->tienNuoc) + L" đ\n";
    s += L"Tiền wifi:   " + money(b->tienWifi) + L" đ\n";
    s += L"──────────────────────────────\n";
    s += L"TỔNG TIỀN:   " + money(b->tongTien) + L" đ\n\n";
    s += L"Hạn thanh toán: " + W(b->hanThanhToan.toDisplay()) + L"\n";
    s += L"Trạng thái: ";
    if (b->daThanhToan) s += L"Đã thanh toán";
    else if (qh > 0) s += L"Chưa thanh toán — quá hạn " + num(qh) + L" ngày";
    else s += L"Chưa thanh toán";
    MessageBoxW(m_hwnd, s.c_str(), L"Chi tiết phiếu thu", MB_OK | MB_ICONINFORMATION);
}

void WinApp::payBill(int maPhieu) {
    PhieuThuTien* b = m_db.findBill(maPhieu);
    if (!b) return;
    if (b->daThanhToan) {
        info(m_hwnd, L"Phiếu thu #" + num(maPhieu) + L" đã được thanh toán.");
        return;
    }
    if (!confirm(m_hwnd, L"Xác nhận khách hàng đã thanh toán phiếu #" + num(maPhieu) + L"\nSố tiền: " +
                             money(b->tongTien) + L"đ ?"))
        return;
    std::string err = m_db.payBill(maPhieu);
    if (!err.empty()) {
        warn(m_hwnd, W(err));
        return;
    }
    commit();
    refresh();
    setDetail(L"Đã thanh toán phiếu #" + num(maPhieu) + L" và xóa khỏi Min-Heap.");
}

void WinApp::changePasswordDialog(int maTK, bool requireOld) {
    TaiKhoan* tk = m_db.tenants.findAccount(maTK);
    if (!tk) return;
    FormDialog f(requireOld ? L"Đổi mật khẩu" : L"Đặt lại mật khẩu", 460);
    f.add(L"Tài khoản", FT_READONLY, W(tk->userName));
    int iOld = requireOld ? f.add(L"Mật khẩu hiện tại", FT_PASSWORD) : -1;
    int iNew = f.add(L"Mật khẩu mới", FT_PASSWORD);
    int iNew2 = f.add(L"Nhập lại mật khẩu mới", FT_PASSWORD);
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        if (d.text(iNew) != d.text(iNew2)) return L"Mật khẩu nhập lại không khớp.";
        return W(m_db.changePassword(maTK, iOld >= 0 ? U8(d.text(iOld)) : "", U8(d.text(iNew)), requireOld));
    };
    if (f.show(m_hwnd) == IDOK) {
        commit();
        info(m_hwnd, L"Đã đổi mật khẩu cho tài khoản \"" + W(tk->userName) + L"\".");
    }
}

void WinApp::managerAccountDialog() {
    FormDialog f(L"Thêm tài khoản quản lý", 480);
    int iUser = f.add(L"Tên đăng nhập", FT_TEXT);
    int iPass = f.add(L"Mật khẩu", FT_PASSWORD);
    int iPass2 = f.add(L"Nhập lại mật khẩu", FT_PASSWORD);
    int iTen = f.add(L"Tên quản lý", FT_TEXT);
    f.setOkText(L"Tạo tài khoản");
    f.onSubmit = [&](FormDialog& d, int) -> std::wstring {
        if (d.text(iPass) != d.text(iPass2)) return L"Mật khẩu nhập lại không khớp.";
        return W(m_db.addManagerAccount(U8(trim(d.text(iUser))), U8(d.text(iPass)), U8(trim(d.text(iTen)))));
    };
    if (f.show(m_hwnd) == IDOK) {
        commit();
        refreshAccounts();
    }
}

void WinApp::aboutDialog() {
    std::wstring s =
        L"HỆ THỐNG QUẢN LÝ PHÒNG TRỌ & HÓA ĐƠN DỊCH VỤ\n"
        L"Đồ án môn Cấu trúc dữ liệu & Giải thuật (C++ / Win32 API)\n\n"
        L"Cấu trúc dữ liệu sử dụng:\n"
        L"  • Danh sách liên kết đôi: danh mục phòng trọ (duyệt Trước / Sau)\n"
        L"  • Bảng băm + DSLK đơn: khách hàng (Mã / CCCD / SĐT), tài khoản\n"
        L"  • Min-Heap: hóa đơn chưa thanh toán theo hạn thanh toán\n\n"
        L"Thuật toán:\n"
        L"  • MergeSort trên DSLK: sắp xếp phòng theo giá / diện tích\n"
        L"  • QuickSort trên mảng: hóa đơn theo tháng / tổng tiền, khách thân thiết\n"
        L"  • Binary Search: tìm phòng theo khoảng giá\n\n"
        L"Dữ liệu lưu tại: " + m_storage.dataDir();
    MessageBoxW(m_hwnd, s.c_str(), L"Giới thiệu", MB_OK | MB_ICONINFORMATION);
}

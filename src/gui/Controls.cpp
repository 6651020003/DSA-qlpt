#include "Controls.h"

namespace ui {

namespace {
HFONT g_font = NULL;
HFONT g_titleFont = NULL;
const wchar_t* FORM_CLASS = L"QLPT_FormDialog";
const int FIELD_BASE = 100;
const int ROW = 34;
const int LABEL_W = 170;

void applyFont(HWND h) { SendMessageW(h, WM_SETFONT, (WPARAM)font(), TRUE); }
}  // namespace

// ================= Chuỗi =================

std::wstring W(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

std::string U8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

std::wstring num(long long v) { return std::to_wstring(v); }

std::wstring money(long long v) {
    bool neg = v < 0;
    std::wstring s = std::to_wstring(neg ? -v : v);
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, L".");
    return neg ? L"-" + s : s;
}

std::wstring trim(const std::wstring& s) {
    size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool parseNumber(const std::wstring& s, long long& out) {
    std::wstring t;
    for (wchar_t c : trim(s)) {
        if (c == L'.' || c == L',' || c == L' ' || c == L'đ') continue;
        if (c < L'0' || c > L'9') return false;
        t += c;
    }
    if (t.empty() || t.size() > 15) return false;
    out = std::stoll(t);
    return true;
}

std::wstring lower(const std::wstring& s) {
    std::wstring r = s;
    if (!r.empty()) CharLowerBuffW(&r[0], (DWORD)r.size());
    return r;
}

// ================= Font =================

void initFonts() {
    if (g_font) return;
    NONCLIENTMETRICSW ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    LOGFONTW lf = ncm.lfMessageFont;
    g_font = CreateFontIndirectW(&lf);
    lf.lfHeight = lf.lfHeight * 3 / 2;
    lf.lfWeight = FW_SEMIBOLD;
    g_titleFont = CreateFontIndirectW(&lf);
}

HFONT font() { return g_font ? g_font : (HFONT)GetStockObject(DEFAULT_GUI_FONT); }
HFONT titleFont() { return g_titleFont ? g_titleFont : font(); }

// ================= Control =================

HWND label(HWND parent, const std::wstring& text, int x, int y, int w, int h, DWORD style) {
    HWND c = CreateWindowExW(0, L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX | style, x, y, w, h,
                             parent, NULL, GetModuleHandleW(NULL), NULL);
    applyFont(c);
    return c;
}

HWND edit(HWND parent, int id, int x, int y, int w, int h, DWORD style) {
    HWND c = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | style,
                             x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    applyFont(c);
    return c;
}

HWND button(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
    HWND c = CreateWindowExW(0, L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, x, y, w,
                             h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    applyFont(c);
    return c;
}

HWND combo(HWND parent, int id, int x, int y, int w, const std::vector<std::wstring>& items, int sel) {
    HWND c = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                             x, y, w, 300, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    applyFont(c);
    for (const std::wstring& it : items) SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)it.c_str());
    if (!items.empty()) SendMessageW(c, CB_SETCURSEL, sel >= 0 && sel < (int)items.size() ? sel : 0, 0);
    return c;
}

HWND checkbox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
    HWND c = CreateWindowExW(0, L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w,
                             h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    applyFont(c);
    return c;
}

std::wstring getText(HWND h) {
    if (!h) return L"";
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(h, &s[0], n + 1);
    s.resize(n);
    return s;
}

void setText(HWND h, const std::wstring& text) {
    if (h) SetWindowTextW(h, text.c_str());
}

int comboSel(HWND h) { return h ? (int)SendMessageW(h, CB_GETCURSEL, 0, 0) : -1; }

bool isChecked(HWND h) { return h && SendMessageW(h, BM_GETCHECK, 0, 0) == BST_CHECKED; }

// ================= Thông báo =================

void info(HWND owner, const std::wstring& msg) {
    MessageBoxW(owner, msg.c_str(), L"Thông báo", MB_OK | MB_ICONINFORMATION);
}

void warn(HWND owner, const std::wstring& msg) {
    MessageBoxW(owner, msg.c_str(), L"Thông báo", MB_OK | MB_ICONWARNING);
}

bool confirm(HWND owner, const std::wstring& msg) {
    return MessageBoxW(owner, msg.c_str(), L"Xác nhận", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
}

// ================= ListView =================

HWND listView(HWND parent, int id) {
    HWND lv = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0,
                              0, 100, 100, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    applyFont(lv);
    return lv;
}

void lvSetColumns(HWND lv, const std::vector<Column>& cols) {
    lvClear(lv);
    while (ListView_DeleteColumn(lv, 0)) {
    }
    for (size_t i = 0; i < cols.size(); ++i) {
        LVCOLUMNW c = {};
        c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        c.fmt = (cols[i].right && i > 0) ? LVCFMT_RIGHT : LVCFMT_LEFT;
        c.cx = cols[i].width;
        c.pszText = const_cast<wchar_t*>(cols[i].title.c_str());
        ListView_InsertColumn(lv, (int)i, &c);
    }
}

int lvAddRow(HWND lv, const std::vector<std::wstring>& cells, LPARAM data) {
    LVITEMW it = {};
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = ListView_GetItemCount(lv);
    it.pszText = const_cast<wchar_t*>(cells.empty() ? L"" : cells[0].c_str());
    it.lParam = data;
    int row = (int)SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&it);
    for (size_t i = 1; i < cells.size(); ++i)
        ListView_SetItemText(lv, row, (int)i, const_cast<wchar_t*>(cells[i].c_str()));
    return row;
}

void lvClear(HWND lv) { ListView_DeleteAllItems(lv); }

LPARAM lvSelectedData(HWND lv) {
    int idx = ListView_GetNextItem(lv, -1, LVNI_SELECTED);
    if (idx < 0) return -1;
    LVITEMW it = {};
    it.mask = LVIF_PARAM;
    it.iItem = idx;
    SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&it);
    return it.lParam;
}

void lvSelectByData(HWND lv, LPARAM data) {
    LVFINDINFOW fi = {};
    fi.flags = LVFI_PARAM;
    fi.lParam = data;
    int idx = (int)SendMessageW(lv, LVM_FINDITEMW, (WPARAM)-1, (LPARAM)&fi);
    if (idx < 0) return;
    ListView_SetItemState(lv, idx, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(lv, idx, FALSE);
}

void lvBeginUpdate(HWND lv) { SendMessageW(lv, WM_SETREDRAW, FALSE, 0); }

void lvEndUpdate(HWND lv) {
    SendMessageW(lv, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(lv, NULL, TRUE);
}

// ================= FormDialog =================

const wchar_t* const FormDialog::STAY = L"\x01";

FormDialog::FormDialog(const std::wstring& title, int width)
    : m_title(title), m_okText(L"Lưu"), m_cancelText(L"Hủy"), m_width(width), m_hwnd(NULL), m_done(false),
      m_suppress(false), m_result(IDCANCEL) {}

int FormDialog::add(const std::wstring& label, FieldType type, const std::wstring& value) {
    Field f;
    f.label = label;
    f.type = type;
    f.value = value;
    f.sel = 0;
    f.check = false;
    f.ctrl = NULL;
    m_fields.push_back(f);
    return (int)m_fields.size() - 1;
}

int FormDialog::addCombo(const std::wstring& label, const std::vector<std::wstring>& items, int sel) {
    int i = add(label, FT_COMBO);
    m_fields[i].items = items;
    m_fields[i].sel = sel;
    return i;
}

int FormDialog::addDate(const std::wstring& label, const Date& d) {
    int i = add(label, FT_DATE);
    m_fields[i].date = d.empty() ? Date::today() : d;
    return i;
}

int FormDialog::addCheck(const std::wstring& label, bool checked) {
    int i = add(label, FT_CHECK);
    m_fields[i].check = checked;
    return i;
}

void FormDialog::addButton(int id, const std::wstring& text) {
    ExtraButton b;
    b.id = id;
    b.text = text;
    m_buttons.push_back(b);
}

std::wstring FormDialog::text(int i) const {
    const Field& f = m_fields[i];
    if (f.type == FT_COMBO) {
        int s = sel(i);
        return s >= 0 && s < (int)f.items.size() ? f.items[s] : L"";
    }
    return m_hwnd && f.ctrl ? getText(f.ctrl) : f.value;
}

void FormDialog::setText(int i, const std::wstring& v) {
    m_fields[i].value = v;
    if (m_hwnd && m_fields[i].ctrl) {
        bool old = m_suppress;
        m_suppress = true;
        ui::setText(m_fields[i].ctrl, v);
        m_suppress = old;
    }
}

bool FormDialog::number(int i, long long& out) const { return parseNumber(text(i), out); }

int FormDialog::sel(int i) const {
    const Field& f = m_fields[i];
    return m_hwnd && f.ctrl ? comboSel(f.ctrl) : f.sel;
}

void FormDialog::setSel(int i, int s) {
    m_fields[i].sel = s;
    if (m_hwnd && m_fields[i].ctrl) {
        bool old = m_suppress;
        m_suppress = true;
        SendMessageW(m_fields[i].ctrl, CB_SETCURSEL, s, 0);
        m_suppress = old;
    }
}

Date FormDialog::date(int i) const {
    const Field& f = m_fields[i];
    if (m_hwnd && f.ctrl) {
        SYSTEMTIME st;
        if (DateTime_GetSystemtime(f.ctrl, &st) == GDT_VALID) return Date(st.wYear, st.wMonth, st.wDay);
        return Date();
    }
    return f.date;
}

void FormDialog::setDate(int i, const Date& d) {
    m_fields[i].date = d;
    if (m_hwnd && m_fields[i].ctrl && !d.empty()) {
        SYSTEMTIME st = {};
        st.wYear = (WORD)d.y;
        st.wMonth = (WORD)d.m;
        st.wDay = (WORD)d.d;
        bool old = m_suppress;
        m_suppress = true;
        DateTime_SetSystemtime(m_fields[i].ctrl, GDT_VALID, &st);
        m_suppress = old;
    }
}

bool FormDialog::checked(int i) const {
    const Field& f = m_fields[i];
    return m_hwnd && f.ctrl ? isChecked(f.ctrl) : f.check;
}

void FormDialog::createControls() {
    int ctrlX = 20 + LABEL_W;
    int ctrlW = m_width - ctrlX - 20;
    int y = 18;
    for (size_t i = 0; i < m_fields.size(); ++i) {
        Field& f = m_fields[i];
        int id = FIELD_BASE + (int)i;
        if (f.type != FT_CHECK) label(m_hwnd, f.label, 20, y + 4, LABEL_W - 8, 20);
        switch (f.type) {
        case FT_COMBO:
            f.ctrl = combo(m_hwnd, id, ctrlX, y, ctrlW, f.items, f.sel);
            break;
        case FT_DATE:
            f.ctrl = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | DTS_SHORTDATEFORMAT,
                                     ctrlX, y, 160, 24, m_hwnd, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
            applyFont(f.ctrl);
            DateTime_SetFormat(f.ctrl, L"dd/MM/yyyy");
            setDate((int)i, f.date);
            break;
        case FT_CHECK:
            f.ctrl = checkbox(m_hwnd, id, f.label, ctrlX, y, ctrlW, 24);
            if (f.check) SendMessageW(f.ctrl, BM_SETCHECK, BST_CHECKED, 0);
            break;
        default: {
            DWORD style = 0;
            if (f.type == FT_PASSWORD) style = ES_PASSWORD;
            if (f.type == FT_READONLY) style = ES_READONLY;
            f.ctrl = edit(m_hwnd, id, ctrlX, y, ctrlW, 24, style);
            ui::setText(f.ctrl, f.value);
            break;
        }
        }
        y += ROW;
    }

    if (!m_note.empty()) {
        HWND n = label(m_hwnd, m_note, 20, y + 2, m_width - 40, 40);
        (void)n;
        y += 46;
    }

    y += 8;
    int bx = 20;
    for (const ExtraButton& b : m_buttons) {
        button(m_hwnd, b.id, b.text, bx, y, 110, 30);
        bx += 118;
    }
    button(m_hwnd, IDCANCEL, m_cancelText, m_width - 20 - 100, y, 100, 30);
    button(m_hwnd, IDOK, m_okText, m_width - 20 - 100 - 8 - 110, y, 110, 30);
}

int FormDialog::show(HWND owner) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = proc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = FORM_CLASS;
        wc.hIcon = LoadIcon(GetModuleHandleW(NULL), MAKEINTRESOURCE(1));
        RegisterClassExW(&wc);
        registered = true;
    }

    int clientH = 18 + (int)m_fields.size() * ROW + (m_note.empty() ? 0 : 46) + 8 + 30 + 18;
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    DWORD exStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    RECT rc = {0, 0, m_width, clientH};
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;

    RECT area;
    if (owner && IsWindowVisible(owner)) GetWindowRect(owner, &area);
    else SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0);
    int x = area.left + (area.right - area.left - w) / 2;
    int y = area.top + (area.bottom - area.top - h) / 2;

    m_done = false;
    m_result = IDCANCEL;
    m_hwnd = CreateWindowExW(exStyle, FORM_CLASS, m_title.c_str(), style, x, y, w, h, owner, NULL,
                             GetModuleHandleW(NULL), this);
    if (!m_hwnd) return IDCANCEL;

    m_suppress = true;
    createControls();
    m_suppress = false;
    if (onChange) changed(-1);

    ShowWindow(m_hwnd, SW_SHOW);
    if (owner) EnableWindow(owner, FALSE);
    for (const Field& f : m_fields) {
        if (f.type != FT_READONLY && f.ctrl) {
            SetFocus(f.ctrl);
            if (f.type != FT_COMBO && f.type != FT_DATE && f.type != FT_CHECK) SendMessageW(f.ctrl, EM_SETSEL, 0, -1);
            break;
        }
    }

    MSG msg;
    while (!m_done) {
        BOOL r = GetMessageW(&msg, NULL, 0, 0);
        if (r <= 0) {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        if (!IsDialogMessageW(m_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (owner) {
        EnableWindow(owner, TRUE);
        SetActiveWindow(owner);
    }
    DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    for (Field& f : m_fields) f.ctrl = NULL;
    return m_result;
}

void FormDialog::collect() {
    for (size_t i = 0; i < m_fields.size(); ++i) {
        Field& f = m_fields[i];
        if (!f.ctrl) continue;
        switch (f.type) {
        case FT_COMBO: f.sel = comboSel(f.ctrl); break;
        case FT_DATE: f.date = date((int)i); break;
        case FT_CHECK: f.check = isChecked(f.ctrl); break;
        default: f.value = getText(f.ctrl); break;
        }
    }
}

void FormDialog::finish(int result) {
    collect();
    m_result = result;
    m_done = true;
}

void FormDialog::submit(int id) {
    if (onSubmit) {
        std::wstring err = onSubmit(*this, id);
        if (err == STAY) return;
        if (!err.empty()) {
            warn(m_hwnd, err);
            return;
        }
    }
    finish(id);
}

void FormDialog::changed(int idx) {
    if (m_suppress || !onChange) return;
    m_suppress = true;
    onChange(*this, idx);
    m_suppress = false;
}

LRESULT CALLBACK FormDialog::proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    FormDialog* self;
    if (msg == WM_NCCREATE) {
        self = (FormDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hwnd = h;
    } else {
        self = (FormDialog*)GetWindowLongPtrW(h, GWLP_USERDATA);
    }
    return self ? self->handle(msg, wp, lp) : DefWindowProcW(h, msg, wp, lp);
}

LRESULT FormDialog::handle(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == IDCANCEL) {
            finish(IDCANCEL);
            return 0;
        }
        if (id >= FIELD_BASE && id < FIELD_BASE + (int)m_fields.size()) {
            if (code == EN_CHANGE || code == CBN_SELCHANGE || code == BN_CLICKED) changed(id - FIELD_BASE);
            return 0;
        }
        if (code == BN_CLICKED) {
            if (id == IDOK) {
                submit(IDOK);
                return 0;
            }
            for (const ExtraButton& b : m_buttons)
                if (b.id == id) {
                    submit(id);
                    return 0;
                }
        }
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lp;
        if (nm->code == DTN_DATETIMECHANGE && (int)nm->idFrom >= FIELD_BASE)
            changed((int)nm->idFrom - FIELD_BASE);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        // Ô chỉ đọc (tự tính) hiển thị nền xám nhạt để phân biệt với ô nhập
        HDC dc = (HDC)wp;
        HWND c = (HWND)lp;
        wchar_t cls[16];
        GetClassNameW(c, cls, 16);
        if (lstrcmpiW(cls, L"Edit") == 0) {
            SetBkColor(dc, RGB(236, 240, 245));
            static HBRUSH br = CreateSolidBrush(RGB(236, 240, 245));
            return (LRESULT)br;
        }
        break;
    }
    case DM_GETDEFID:
        return MAKELRESULT(IDOK, DC_HASDEFID);
    case WM_CLOSE:
        finish(IDCANCEL);
        return 0;
    }
    return DefWindowProcW(m_hwnd, msg, wp, lp);
}

}  // namespace ui

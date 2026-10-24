// Controls.h - [TV2] Tiện ích giao diện Win32: tạo control, ListView, Form nhập liệu dạng modal
#ifndef CONTROLS_H
#define CONTROLS_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <functional>
#include "../core/Models.h"

namespace ui {

// ----- Chuỗi & định dạng -----
std::wstring W(const std::string& utf8);
std::string U8(const std::wstring& w);
std::wstring num(long long v);
std::wstring money(long long v);              // 2500000 -> "2.500.000"
std::wstring trim(const std::wstring& s);
bool parseNumber(const std::wstring& s, long long& out);  // chấp nhận "2.500.000", "2,500,000"
std::wstring lower(const std::wstring& s);

// ----- Font -----
void initFonts();
HFONT font();
HFONT titleFont();

// ----- Tạo control -----
HWND label(HWND parent, const std::wstring& text, int x, int y, int w, int h, DWORD style = 0);
HWND edit(HWND parent, int id, int x, int y, int w, int h, DWORD style = 0);
HWND button(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h);
HWND combo(HWND parent, int id, int x, int y, int w, const std::vector<std::wstring>& items, int sel = 0);
HWND checkbox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h);
std::wstring getText(HWND h);
void setText(HWND h, const std::wstring& text);
int comboSel(HWND h);
bool isChecked(HWND h);

// ----- Hộp thông báo -----
void info(HWND owner, const std::wstring& msg);
void warn(HWND owner, const std::wstring& msg);
bool confirm(HWND owner, const std::wstring& msg);

// ----- ListView (bảng dữ liệu) -----
struct Column {
    std::wstring title;
    int width;
    bool right;  // canh phải (cột số tiền)
};
HWND listView(HWND parent, int id);
void lvSetColumns(HWND lv, const std::vector<Column>& cols);
int lvAddRow(HWND lv, const std::vector<std::wstring>& cells, LPARAM data);
void lvClear(HWND lv);
LPARAM lvSelectedData(HWND lv);   // -1 nếu chưa chọn dòng nào
void lvSelectByData(HWND lv, LPARAM data);
void lvBeginUpdate(HWND lv);
void lvEndUpdate(HWND lv);

// ----- Form nhập liệu modal (thay cho file .rc dialog) -----
enum FieldType { FT_TEXT, FT_NUMBER, FT_PASSWORD, FT_READONLY, FT_COMBO, FT_DATE, FT_CHECK };

class FormDialog {
public:
    // Trả về từ onSubmit: chuỗi rỗng = đóng form; STAY = giữ form không báo lỗi; khác = báo lỗi và giữ form
    static const wchar_t* const STAY;

    typedef std::function<void(FormDialog&, int)> ChangeHandler;           // idx = -1 khi form vừa mở
    typedef std::function<std::wstring(FormDialog&, int)> SubmitHandler;  // id = IDOK hoặc id nút phụ

    explicit FormDialog(const std::wstring& title, int width = 500);

    int add(const std::wstring& label, FieldType type, const std::wstring& value = L"");
    int addCombo(const std::wstring& label, const std::vector<std::wstring>& items, int sel = 0);
    int addDate(const std::wstring& label, const Date& d);
    int addCheck(const std::wstring& label, bool checked);
    void addButton(int id, const std::wstring& text);  // nút phụ bên trái (id từ 3 đến 99)
    void setOkText(const std::wstring& t) { m_okText = t; }
    void setCancelText(const std::wstring& t) { m_cancelText = t; }
    void setNote(const std::wstring& t) { m_note = t; }

    ChangeHandler onChange;
    SubmitHandler onSubmit;

    int show(HWND owner);  // trả về IDOK, IDCANCEL hoặc id nút phụ

    std::wstring text(int i) const;
    void setText(int i, const std::wstring& v);
    bool number(int i, long long& out) const;
    int sel(int i) const;
    void setSel(int i, int s);
    Date date(int i) const;
    void setDate(int i, const Date& d);
    bool checked(int i) const;
    HWND hwnd() const { return m_hwnd; }

private:
    struct Field {
        std::wstring label;
        FieldType type;
        std::wstring value;
        std::vector<std::wstring> items;
        int sel;
        Date date;
        bool check;
        HWND ctrl;
    };
    struct ExtraButton {
        int id;
        std::wstring text;
    };

    std::wstring m_title, m_okText, m_cancelText, m_note;
    int m_width;
    std::vector<Field> m_fields;
    std::vector<ExtraButton> m_buttons;
    HWND m_hwnd;
    bool m_done;
    bool m_suppress;
    int m_result;

    static LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);
    void createControls();
    void collect();
    void submit(int id);
    void finish(int result);
    void changed(int idx);
};

}  // namespace ui

#endif

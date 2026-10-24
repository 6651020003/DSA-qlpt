// WinApp.h - [TV1 & TV2] Cửa sổ chính, menu, các màn hình và hộp thoại nghiệp vụ
#ifndef WIN_APP_H
#define WIN_APP_H

#include <windows.h>
#include <string>
#include <vector>
#include "../services/AppData.h"
#include "../services/FileStorage.h"
#include "../services/Sorter.h"

class WinApp {
public:
    WinApp(HINSTANCE inst, AppData& db, FileStorage& storage);
    int run(int nCmdShow);

private:
    enum View { V_NONE, V_ROOMS, V_CUSTOMERS, V_RENTALS, V_BILLS, V_UNPAID, V_REVENUE, V_LOYAL, V_ACCOUNTS, V_MY };

    HINSTANCE m_inst;
    AppData& m_db;
    FileStorage& m_storage;

    HWND m_hwnd, m_title, m_list, m_status;
    HACCEL m_accel;
    int m_userId, m_role, m_maQL;
    bool m_logout;

    View m_view;
    std::vector<HWND> m_ctrls;
    int m_x[2];
    int m_listTop;
    int m_defBtn;

    // Trạng thái màn hình Phòng trọ
    bool m_roomFiltered;
    RoomFilter m_roomFilter;
    int m_curRoom;
    int m_roomSortKey;
    bool m_roomSortAsc;
    bool m_roomSorted;
    // Trạng thái màn hình Hóa đơn
    int m_billSortKey;
    bool m_billSortAsc;

    // ----- Vòng đời -----
    bool login();
    void registerDialog();
    void createMainWindow(int nCmdShow);
    HMENU buildMenu();
    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);
    void layout();
    void onCommand(int id, int code);
    LRESULT onNotify(NMHDR* nm);
    bool commit();
    void setStatus(const std::wstring& text);
    void setDetail(const std::wstring& text);
    bool isManager() const { return m_role == ROLE_QUAN_LY; }
    std::wstring managerName(int maQL) const;
    std::wstring customerName(int maKH) const;

    // ----- Dựng màn hình -----
    void switchView(View v);
    void clearView();
    HWND track(HWND h);
    HWND tbButton(int row, int id, const wchar_t* text, int w = 100);
    HWND tbLabel(int row, const wchar_t* text, int w);
    HWND tbEdit(int row, int id, int w, const std::wstring& text = L"");
    HWND tbCombo(int row, int id, int w, const std::vector<std::wstring>& items, int sel = 0);
    HWND tbCheck(int row, int id, const wchar_t* text, int w);
    void tbGap(int row, int w) { m_x[row] += w; }
    HWND ctl(int id) const { return GetDlgItem(m_hwnd, id); }
    void refresh();

    void buildRooms();
    void refreshRooms();
    void roomCommand(int id);
    void showRoomDetail(int maPhong);

    void buildCustomers();
    void refreshCustomers();
    void customerCommand(int id);

    void buildRentals();
    void refreshRentals();
    void rentalCommand(int id);

    void buildBills();
    void refreshBills();
    void billCommand(int id);

    void buildUnpaid();
    void refreshUnpaid();

    void buildRevenue();
    void refreshRevenue();

    void buildLoyal();
    void refreshLoyal();

    void buildAccounts();
    void refreshAccounts();
    void accountCommand(int id);

    void buildMy();
    void refreshMy();

    // ----- Hộp thoại nghiệp vụ -----
    void roomDialog(int maPhong);
    void importRooms();
    void customerDialog(int maKH);
    void rentDialog(int maPhong);
    void returnDialog(int maPhieuTTP);
    void billDialog(int maPhieuTTP);
    void billDetail(int maPhieu);
    void payBill(int maPhieu);
    void changePasswordDialog(int maTK, bool requireOld);
    void managerAccountDialog();
    void aboutDialog();
};

#endif

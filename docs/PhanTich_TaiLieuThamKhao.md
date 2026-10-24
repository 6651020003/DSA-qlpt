# Phân tích tài liệu tham khảo

> Nguồn: *Quản lý Phòng Trọ – Thực hành môn Công nghệ phần mềm, Nhóm 10 (HUFLIT)*.
> Bản gốc là web app (SQL Server). File này chỉ ghi lại những phần **đem sang được** đồ án DSA C++/Win32 và cách áp dụng.

---

## 1. Tác nhân & phân quyền

| Vai trò | Quyền |
|---|---|
| **Quản lý** (chủ trọ) | Toàn quyền: phòng, khách hàng, thuê/trả, phiếu thu, thống kê, tài khoản |
| **Khách hàng** | Xem danh sách phòng, tìm kiếm phòng, xem chi tiết; đăng ký / đăng nhập / sửa tài khoản |

Sơ đồ màn hình tổng quát (Hình 18):

```
Đăng ký ──► Đăng nhập ──┬──► Trang chủ Khách hàng (xem + tìm phòng)
                        └──► Trang chủ Quản lý
                               ├─ Thêm / Sửa / Xóa phòng trọ
                               ├─ Lập phiếu thuê – trả phòng
                               ├─ Lập phiếu thu tiền
                               ├─ Xem thông tin khách hàng
                               ├─ Xem doanh thu
                               ├─ Chưa thanh toán
                               └─ Khách hàng thân thiết
```

---

## 2. Mô hình dữ liệu (Sơ đồ logic – Hình 17, Bảng 40–45)

### PhongTro
| Trường | Kiểu gốc | Kiểu C++ | Ghi chú |
|---|---|---|---|
| MaPhong | INT, PK | `int` | Không trùng, không được sửa |
| GiaThue | INT | `long long` | VNĐ/tháng |
| DienTich | INT | `int` | m² |
| VatChat | NVARCHAR(30) | `std::string` | VD: "1 giường, 1 quạt, 1 tủ" |
| TrangThai | NVARCHAR(15) | `enum` / `std::string` | `Trống` / `Đã thuê` |
| Hinh | NVARCHAR(MAX) | `std::string` | Đường dẫn ảnh (tuỳ chọn) |

### TaiKhoan
| Trường | Kiểu gốc | Ghi chú |
|---|---|---|
| MaTaiKhoan | INT, PK | |
| UserName | NVARCHAR(30) | Không trùng → khóa băm |
| MatKhau | VARCHAR(20–30) | |
| Role | NVARCHAR(20) | `QuanLy` / `KhachHang` |

### QuanLy
| Trường | Kiểu gốc |
|---|---|
| MaQuanLy | INT, PK |
| TenQuanLy | NVARCHAR(30) |

### KhachHang
| Trường | Kiểu gốc | Ghi chú |
|---|---|---|
| MaKhachHang | INT, PK | Khóa băm chính |
| TenKhachHang | NVARCHAR(30) | ≤ 30 ký tự (TS4) |
| DiaChi | NVARCHAR(50) | ≤ 50 ký tự (TS5) |
| CCCD | VARCHAR(20) | Khóa băm phụ |
| SDT | VARCHAR(15) | ≤ 15 ký tự (TS3), khóa băm phụ |
| MaTaiKhoan | INT, FK → TaiKhoan | |

### PhieuThueTraPhong
| Trường | Kiểu gốc | Ghi chú |
|---|---|---|
| MaPhieuTTP | INT, PK | |
| CCCD, DiaChi, SDT, TenKhachHang | | Sao chép thông tin khách lúc lập phiếu |
| NgayThue | DATE | |
| NgayTra | DATE, **nullable** | Rỗng = đang thuê |
| MaQuanLy | INT, FK | |
| MaPhong | INT, FK | |

### PhieuThuTien
| Trường | Kiểu gốc | Ghi chú |
|---|---|---|
| MaPhieuThuTien | INT, PK | |
| TongTien | INT | Tự tính |
| HanThanhToan | DATE | Dùng làm độ ưu tiên trong Min-Heap |
| TienDien, TienNuoc, TienWifi | INT | |
| MaQuanLy, MaKhachHang, MaPhieuTTP | INT, FK | |

Biểu mẫu BM05 (Hình 4/24) còn có thêm các trường hiển thị: **Giá phòng** (lấy từ phòng), **Tháng**, **Số kWh điện**, **Số khối nước**, **Trạng thái** (`Đã thanh toán` / `Chưa thanh toán`). → Bổ sung vào struct: `Thang`, `Nam`, `SoDien`, `SoNuoc`, `TienPhong`, `DaThanhToan`.

---

## 3. Bảng tham số (Bảng 46) → hằng số trong code

| Mã | Giá trị | Ý nghĩa | Hằng số C++ |
|---|---|---|---|
| TS1 | 2612 | Đơn giá điện (đ/kWh) | `DON_GIA_DIEN` |
| TS2 | 13000 | Đơn giá nước (đ/m³) | `DON_GIA_NUOC` |
| TS3 | 15 | Độ dài SĐT tối đa | `MAX_SDT` |
| TS4 | 30 | Độ dài tên tối đa | `MAX_TEN` |
| TS5 | 50 | Độ dài địa chỉ tối đa | `MAX_DIACHI` |

**Công thức phiếu thu:**

```
TienDien = SoDien × 2612
TienNuoc = SoNuoc × 13000
TongTien = TienPhong + TienDien + TienNuoc + TienWifi
```

---

## 4. Quy định nghiệp vụ (Bảng 2)

| Mã | Nội dung | Cách áp dụng trong app |
|---|---|---|
| QD01 | Đổi thông tin phòng phải qua yêu cầu chủ trọ | Chỉ role Quản lý được Thêm/Sửa/Xóa phòng |
| QD02 | Đổi thông tin khách phải qua yêu cầu khách | Chỉ Quản lý sửa; có hộp thoại xác nhận |
| QD03 | Lập phiếu thu tiền mỗi tháng | Không cho lập 2 phiếu cùng tháng/năm cho 1 phiếu thuê |
| QD04 | Đặt cọc giữ phòng = 50% giá thuê 1 tháng | Hiển thị tiền cọc khi lập phiếu thuê |
| QD05 | Trả phòng phải đảm bảo vật chất nguyên vẹn | Ghi chú/xác nhận khi lập phiếu trả |

---

## 5. Quy tắc kiểm tra dữ liệu (từ đặc tả Use case)

- **Thêm phòng:** không trùng `MaPhong`; Giá thuê, Diện tích > 0.
- **Sửa phòng:** không được sửa `MaPhong`.
- **Xóa phòng:** phải xác nhận; không xóa phòng đang `Đã thuê`.
- **Xóa khách hàng:** chỉ được xóa khi khách **không còn** phiếu thuê/phiếu thu liên quan; phải xác nhận.
- **Lập phiếu thuê:** chỉ chọn phòng `Trống`; sau khi lưu → phòng thành `Đã thuê`.
- **Lập phiếu trả:** chọn phiếu thuê đang hiệu lực (NgayTra rỗng), nhập NgayTra → phòng thành `Trống`.
- **Lập phiếu thu:** chỉ cho các phòng đang thuê; tiền điện/nước/tổng tự tính, ô chỉ đọc.
- **Đăng ký:** UserName không trùng; Mật khẩu = Nhập lại mật khẩu; kiểm tra độ dài theo TS3–TS5.
- **Đăng nhập:** khớp UserName + MatKhau → chuyển màn hình theo Role.

---

## 6. Chức năng thống kê

| Use case | Đầu vào | Kết quả | CTDL/Thuật toán dùng |
|---|---|---|---|
| UC19 – Xem doanh thu | Tháng/Năm (hoặc cả năm) | Danh sách phiếu thu **đã thanh toán** + tổng doanh thu | Duyệt danh sách + MergeSort theo tháng |
| UC20 – Chưa thanh toán | Tháng hiện tại | Số phòng, Tên khách, Số tiền còn thiếu | **Min-Heap** theo HanThanhToan (nợ lâu nhất lên đầu) |
| UC21 – Xem phiếu thuê | – | Mã phiếu, Ngày thuê, Ngày trả, Tên khách, SĐT, CCCD, Địa chỉ | Danh sách phiếu |
| UC22 – Khách hàng thân thiết | Tiêu chí: số tháng thuê / số lần thuê | Mã KH, Tên KH, Số tháng đã thuê | Gom theo khách (Hash) + Sort giảm dần |
| UC23 – Tìm kiếm phòng | Khoảng giá, Diện tích, Tiện nghi | Số phòng, Giá, Diện tích, Vật chất | Sort + **Binary Search** theo khoảng |

---

## 7. Danh sách màn hình → cửa sổ Win32

| # | Màn hình (PDF) | Thành phần Win32 |
|---|---|---|
| 11 | Đăng nhập | Dialog: 2 Edit (User, Pass) + 2 Button (Đăng nhập, Đăng ký) |
| 12 | Đăng ký | Dialog: 7 Edit (User, Pass, Nhập lại, Tên, CCCD, Địa chỉ, SĐT) + Button |
| 1 | Trang chủ Khách hàng | ListView phòng + ô tìm giá (từ – đến) + Đăng xuất |
| 2 | Trang chủ Quản lý | Menu trái/Menu bar + ListView phòng + ô tìm giá + nút Thêm/Sửa/Xóa |
| 3, 4 | Sửa / Thêm phòng | Dialog: MaPhong, GiaThue, DienTich, VatChat, TrangThai (ComboBox), Hinh |
| 5 | Lập phiếu thuê | Dialog: MaPhieu, CCCD, Địa chỉ, SĐT, Ngày thuê, Tên KH, Ngày trả, Mã QL, Mã phòng (ComboBox) |
| 6 | Lập phiếu thu tiền | Dialog: MaPhieu, MaQL, MaKH, MaPhieuTTP (Combo), Giá phòng*, Tháng, Số điện, Tiền điện*, Số nước, Tiền nước*, Tiền mạng, Tổng*, Trạng thái (`*` = chỉ đọc, tự tính) |
| 7 | Quản lý khách hàng | ListView: Mã KH, Tên, Địa chỉ, CCCD, SĐT, Mã TK + Thêm/Sửa/Xóa |
| 8 | Xem doanh thu | Ô nhập tháng + nút Lọc; ListView phiếu thu; nhãn "Tổng doanh thu" |
| 9 | Chưa thanh toán | ListView: Tổng tiền, Tháng, Điện, Nước, Mạng, Trạng thái, Tên KH, CCCD, Tên QL |
| 10 | Khách hàng thân thiết | ListView: Mã KH, Tên KH, Số tháng đã thuê |

---

## 8. Ánh xạ sang cấu trúc dữ liệu của đồ án

| Thực thể | Lưu trữ trong bộ nhớ | Thao tác chính |
|---|---|---|
| PhongTro | **DSLK đôi** `RoomList` | Thêm/xóa/sửa O(1) khi có con trỏ node, duyệt tiến/lùi |
| KhachHang | **Bảng băm (chaining)** `TenantHash` – khóa MaKH, chỉ mục phụ CCCD/SĐT | Tra cứu O(1) trung bình |
| TaiKhoan | **Bảng băm** – khóa UserName | Đăng nhập O(1) |
| PhieuThuTien chưa thanh toán | **Min-Heap** `BillQueue` – khóa (HanThanhToan, số ngày nợ) | Lấy hóa đơn nợ lâu nhất O(log n) |
| Sắp xếp phòng / hóa đơn | `Sorter`: MergeSort trên DSLK, QuickSort trên mảng | Giá thuê, Diện tích / Tháng, Tổng tiền |
| Tìm phòng theo khoảng | Sort + Binary Search (lower/upper bound) | Lọc phòng trống theo giá/diện tích |

---

## 9. File dữ liệu CSV dự kiến (`data/`)

| File | Cột |
|---|---|
| `PhongTro.csv` | MaPhong,GiaThue,DienTich,VatChat,TrangThai,Hinh |
| `TaiKhoan.csv` | MaTaiKhoan,UserName,MatKhau,Role |
| `QuanLy.csv` | MaQuanLy,TenQuanLy,MaTaiKhoan *(bổ sung MaTaiKhoan để biết tài khoản đăng nhập thuộc quản lý nào)* |
| `KhachHang.csv` | MaKhachHang,TenKhachHang,DiaChi,CCCD,SDT,MaTaiKhoan |
| `PhieuThueTraPhong.csv` | MaPhieuTTP,TenKhachHang,CCCD,SDT,DiaChi,NgayThue,NgayTra,MaQuanLy,MaPhong |
| `PhieuThuTien.csv` | MaPhieuThuTien,Thang,Nam,TienPhong,SoDien,SoNuoc,TienDien,TienNuoc,TienWifi,TongTien,HanThanhToan,DaThanhToan,MaQuanLy,MaKhachHang,MaPhieuTTP |

Định dạng ngày: `YYYY-MM-DD`. Mã hóa: UTF-8.

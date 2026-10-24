# Hệ thống Quản lý Phòng trọ & Hóa đơn Dịch vụ

Ứng dụng Windows (C++ / Win32 API, không dùng thư viện ngoài) quản lý phòng trọ, khách thuê, phiếu thuê/trả phòng và hóa đơn điện – nước – wifi hằng tháng.

---

## 1. Cài đặt

### 1.1. Yêu cầu

| Thành phần | Yêu cầu |
|---|---|
| Hệ điều hành | Windows 10 / 11 |
| Trình biên dịch | **MinGW-w64 g++ ≥ 9** (khuyên dùng MSYS2 UCRT64) hoặc **Visual Studio 2019/2022** |

Kiểm tra g++ đã có trong `PATH`:

```bash
g++ --version
```

### 1.2. Biên dịch bằng MinGW-w64 (khuyên dùng)

Tại thư mục gốc `DSAQuanLyPhongTro/`, chạy:

```bash
build.bat
```

Kết quả: file `QuanLyPhongTro.exe` nằm cạnh thư mục `data/`. File exe được link tĩnh nên có thể chép sang máy khác mà không cần cài MinGW.

Nếu muốn tự gõ lệnh build:

```bash
windres res/app.rc -O coff -o build/app.res.o
```

```bash
g++ -std=c++17 -O2 -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 -municode -mwindows -static src/main.cpp src/core/Models.cpp src/core/RoomList.cpp src/core/TenantHash.cpp src/core/BillQueue.cpp src/services/AppData.cpp src/services/BillingService.cpp src/services/Sorter.cpp src/services/FileStorage.cpp src/gui/Controls.cpp src/gui/WinApp.cpp build/app.res.o -o QuanLyPhongTro.exe -lcomctl32 -lcomdlg32 -lgdi32 -luser32
```

### 1.3. Biên dịch bằng Visual Studio

1. Tạo project **Empty Project** (C++), thêm toàn bộ file trong `src/` và `res/app.rc`.
2. `Project → Properties`:
   - `Advanced → Character Set` = **Use Unicode Character Set**
   - `C/C++ → Command Line` thêm `/utf-8` (mã nguồn lưu UTF-8 có tiếng Việt)
   - `Linker → System → SubSystem` = **Windows**
3. Build (**Ctrl + Shift + B**), rồi chép thư mục `data/` vào cạnh file `.exe` (ví dụ `x64/Debug/`).

### 1.4. Chạy kiểm thử (tùy chọn)

```bash
tests\run_tests.bat
```

Chạy hơn 500 phép kiểm tra cho DSLK đôi, bảng băm, Min-Heap, sắp xếp, tìm kiếm và các nghiệp vụ. Bộ test làm việc trên bản sao dữ liệu trong thư mục tạm, không sửa `data/`.

---

## 2. Sử dụng

### 2.1. Khởi động & đăng nhập

Chạy `QuanLyPhongTro.exe`. Dữ liệu mẫu có sẵn các tài khoản:

| Tài khoản | Mật khẩu | Vai trò |
|---|---|---|
| `admin` | `admin123` | Quản lý |
| `quanly2` | `quanly123` | Quản lý |
| `khach01` | `123456` | Khách hàng (Lê Minh Anh, phòng 101) |
| `khach02` | `123456` | Khách hàng (Phạm Thu Hà, phòng 102) |

- Khách hàng mới bấm **Đăng ký...** ở màn hình đăng nhập. Nếu đã từng thuê phòng, nhập đúng CCCD để liên kết với hồ sơ cũ.
- Nếu thư mục `data/` trống, chương trình tự tạo tài khoản `admin` / `admin123`.
- **Hệ thống → Đổi mật khẩu** để đổi mật khẩu sau khi đăng nhập.

### 2.2. Quản lý phòng trọ (menu **Phòng trọ**)

| Thao tác | Cách làm |
|---|---|
| Thêm / Sửa / Xóa | Bấm nút tương ứng (hoặc nháy đúp một dòng để sửa). Mã phòng không được trùng, không sửa được; không xóa được phòng đang có người thuê. |
| Import CSV | **Import CSV...**, chọn file có cột `MaPhong,GiaThue,DienTich,VatChat,TrangThai,Hinh`. Dòng trùng mã hoặc sai dữ liệu sẽ bị bỏ qua. |
| Tìm kiếm | Nhập khoảng **Giá thuê** và/hoặc **Diện tích**, tick **Chỉ phòng trống** nếu cần, bấm **Tìm kiếm** (hoặc Enter). **Bỏ lọc** để xem lại toàn bộ. |
| Sắp xếp | Chọn tiêu chí (Mã phòng / Giá thuê / Diện tích) và chiều, bấm **Sắp xếp**; hoặc bấm vào tiêu đề cột. |
| Duyệt | **◀ Trước / Sau ▶** đi qua từng phòng; thanh trạng thái hiện phòng liền trước / liền sau. |
| Cho thuê | Chọn một phòng trống rồi bấm **Cho thuê...** |

### 2.3. Khách hàng (menu **Khách hàng**)

- **Thêm / Sửa / Xóa** khách hàng. Chỉ xóa được khách chưa có phiếu thuê hay phiếu thu.
- Ô **Tra cứu**: nhập CCCD, SĐT hoặc Mã KH để tìm ngay; nhập một phần tên để tìm theo tên.

### 2.4. Thuê / trả phòng (menu **Thuê - Trả**)

**Lập phiếu thuê**
1. Chọn phòng trống; giá thuê và tiền cọc 50% hiện tự động.
2. Nhập **CCCD**. Khách cũ sẽ tự điền tên, SĐT, địa chỉ; khách mới thì nhập đủ thông tin, hệ thống tự tạo hồ sơ.
3. Chọn ngày thuê, bấm **Tạo phiếu**. Phòng tự chuyển sang **Đã thuê**.

**Lập phiếu trả**
1. Chọn phiếu thuê đang hiệu lực (hoặc nháy đúp phiếu trong danh sách).
2. Xem số hóa đơn còn nợ, chọn ngày trả, tick **Đã kiểm tra vật chất phòng**.
3. Bấm **Xác nhận trả**. Phòng tự chuyển về **Trống**.

### 2.5. Phiếu thu tiền (menu **Hóa đơn**)

1. **Lập phiếu thu...**, chọn phiếu thuê, nhập **Tháng / Năm**, **số kWh điện**, **số m³ nước**, **tiền wifi**.
2. Tiền điện (2.612đ/kWh), tiền nước (13.000đ/m³) và **Tổng tiền** được tính ngay khi nhập.
3. Chọn hạn thanh toán và trạng thái, bấm **Tạo phiếu**. Mỗi phòng chỉ lập được một phiếu cho mỗi tháng.

Trên danh sách phiếu thu:
- Lọc theo tháng, năm, trạng thái; sắp xếp theo Tháng / Tổng tiền / Mã phiếu.
- **Thanh toán**: xác nhận khách đã trả tiền.
- **Chi tiết** (hoặc nháy đúp): xem bảng kê tiền phòng, điện, nước, wifi.

### 2.6. Thống kê (menu **Thống kê**)

| Báo cáo | Cách xem |
|---|---|
| Doanh thu | Nhập năm, chọn **Cả năm** (bảng 12 tháng + dòng tổng) hoặc một tháng (danh sách phiếu đã thu), bấm **Xem**. |
| Chưa thanh toán | Danh sách phiếu còn nợ, phiếu nợ lâu nhất ở trên cùng, kèm số ngày quá hạn và mức độ khẩn cấp. Nhập tháng/năm để lọc; chọn phiếu rồi bấm **Thanh toán**. |
| Khách hàng thân thiết | Chọn tiêu chí (số tháng thuê / số lần thuê / tổng tiền đã trả) và giá trị tối thiểu, bấm **Xem**. |

### 2.7. Tài khoản (menu **Tài khoản**, chỉ Quản lý)

Thêm tài khoản quản lý, đặt lại mật khẩu, xóa tài khoản (không xóa được tài khoản đang đăng nhập hoặc quản lý cuối cùng).

### 2.8. Dành cho khách hàng

Sau khi đăng nhập, khách hàng thấy:
- **Phòng trọ**: xem và tìm kiếm phòng.
- **Của tôi**: phòng đang thuê, danh sách hóa đơn và số tiền còn nợ; cập nhật thông tin cá nhân, đổi mật khẩu.

### 2.9. Lưu dữ liệu & phím tắt

- Dữ liệu tự lưu vào `data/*.csv` sau mỗi thao tác và khi thoát. **Ctrl + S** để lưu thủ công, **F5** để làm mới.
- Có thể mở file CSV bằng Excel, nhưng hãy **đóng ứng dụng trước khi sửa** và lưu lại dạng *CSV UTF-8*.

---

## 3. Lỗi thường gặp

| Hiện tượng | Cách khắc phục |
|---|---|
| `g++` không được nhận diện | Thêm thư mục `bin` của MinGW-w64 (ví dụ `C:\msys64\ucrt64\bin`) vào `PATH`, mở lại terminal. |
| `windres` không được nhận diện | `windres` đi kèm MinGW-w64; kiểm tra lại `PATH` như trên. |
| Mở app không thấy dữ liệu | Thư mục `data/` phải nằm cạnh file `.exe`. |
| Báo "Không ghi được dữ liệu" | Đóng file CSV đang mở trong Excel rồi thao tác lại. |
| Tiếng Việt trong CSV bị lỗi font | Lưu file với mã hóa **UTF-8**. |

// Sorter.h - [TV5] Thuật toán sắp xếp (MergeSort trên DSLK, QuickSort trên mảng)
//            và tìm kiếm (Binary Search theo khoảng giá)
#ifndef SORTER_H
#define SORTER_H

#include "../core/Models.h"
#include "../core/DynArray.h"
#include "../core/RoomList.h"

enum RoomSortKey { ROOM_BY_MA = 0, ROOM_BY_GIA = 1, ROOM_BY_DIEN_TICH = 2 };
enum BillSortKey { BILL_BY_THANG = 0, BILL_BY_TONG_TIEN = 1, BILL_BY_MA = 2 };

// Điều kiện lọc phòng: giá trị -1 nghĩa là không giới hạn
struct RoomFilter {
    long long giaMin, giaMax;
    int dtMin, dtMax;
    bool chiPhongTrong;
    RoomFilter() : giaMin(-1), giaMax(-1), dtMin(-1), dtMax(-1), chiPhongTrong(false) {}
};

class Sorter {
public:
    // ----- MergeSort trực tiếp trên DSLK đôi: O(n log n), ổn định, chỉ nối lại con trỏ -----
    static void mergeSortRooms(RoomList& list, RoomSortKey key, bool asc);

    // ----- QuickSort tổng quát trên mảng động: O(n log n) trung bình -----
    template <typename T, typename Less>
    static void quickSort(DynArray<T>& a, Less less) {
        if (a.size() > 1) quickSortRange(a, 0, a.size() - 1, less);
    }

    static void quickSortRooms(DynArray<PhongTro*>& a, RoomSortKey key, bool asc);
    static void quickSortBills(DynArray<PhieuThuTien*>& a, BillSortKey key, bool asc);

    // ----- Binary Search trên mảng đã sắp tăng dần theo key -----
    static long long roomKey(const PhongTro& p, RoomSortKey key);
    static int lowerBound(const DynArray<PhongTro*>& a, RoomSortKey key, long long value);  // phần tử đầu tiên >= value
    static int upperBound(const DynArray<PhongTro*>& a, RoomSortKey key, long long value);  // phần tử đầu tiên >  value

    // Tìm phòng theo khoảng giá / diện tích / trạng thái.
    // B1: QuickSort theo giá. B2: Binary Search cắt đoạn [giaMin, giaMax]. B3: lọc diện tích, trạng thái.
    static void searchRooms(const RoomList& list, const RoomFilter& f, DynArray<PhongTro*>& out);

private:
    template <typename T, typename Less>
    static void quickSortRange(DynArray<T>& a, int lo, int hi, Less less) {
        while (lo < hi) {
            // Chọn chốt bằng trung vị của 3 phần tử
            int mid = lo + (hi - lo) / 2;
            if (less(a[mid], a[lo])) a.swap(mid, lo);
            if (less(a[hi], a[lo])) a.swap(hi, lo);
            if (less(a[hi], a[mid])) a.swap(hi, mid);
            T pivot = a[mid];

            // Phân hoạch Hoare
            int i = lo, j = hi;
            while (i <= j) {
                while (less(a[i], pivot)) ++i;
                while (less(pivot, a[j])) --j;
                if (i <= j) {
                    a.swap(i, j);
                    ++i;
                    --j;
                }
            }

            // Đệ quy phần nhỏ, lặp phần lớn để giới hạn độ sâu ngăn xếp O(log n)
            if (j - lo < hi - i) {
                if (lo < j) quickSortRange(a, lo, j, less);
                lo = i;
            } else {
                if (i < hi) quickSortRange(a, i, hi, less);
                hi = j;
            }
        }
    }

    static RoomNode* mergeSortNodes(RoomNode* head, RoomSortKey key, bool asc);
    static RoomNode* mergeNodes(RoomNode* a, RoomNode* b, RoomSortKey key, bool asc);
};

#endif

#include "Sorter.h"

long long Sorter::roomKey(const PhongTro& p, RoomSortKey key) {
    switch (key) {
    case ROOM_BY_GIA: return p.giaThue;
    case ROOM_BY_DIEN_TICH: return p.dienTich;
    default: return p.maPhong;
    }
}

// ---------------- MergeSort trên DSLK đôi ----------------

// Trộn 2 dãy đã sắp (chỉ dùng con trỏ next). Lấy từ a khi bằng nhau để giữ tính ổn định.
RoomNode* Sorter::mergeNodes(RoomNode* a, RoomNode* b, RoomSortKey key, bool asc) {
    RoomNode dummy{PhongTro()};
    RoomNode* tail = &dummy;
    while (a && b) {
        long long ka = roomKey(a->data, key), kb = roomKey(b->data, key);
        bool takeA = asc ? (ka <= kb) : (ka >= kb);
        if (takeA) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = a ? a : b;
    return dummy.next;
}

RoomNode* Sorter::mergeSortNodes(RoomNode* head, RoomSortKey key, bool asc) {
    if (!head || !head->next) return head;
    // Tìm giữa danh sách bằng con trỏ chậm / nhanh
    RoomNode* slow = head;
    RoomNode* fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    RoomNode* right = slow->next;
    slow->next = nullptr;
    RoomNode* left = mergeSortNodes(head, key, asc);
    right = mergeSortNodes(right, key, asc);
    return mergeNodes(left, right, key, asc);
}

void Sorter::mergeSortRooms(RoomList& list, RoomSortKey key, bool asc) {
    if (list.m_size < 2) return;
    list.m_head = mergeSortNodes(list.m_head, key, asc);
    // Nối lại con trỏ prev và cập nhật tail
    RoomNode* prev = nullptr;
    for (RoomNode* n = list.m_head; n; n = n->next) {
        n->prev = prev;
        prev = n;
    }
    list.m_tail = prev;
}

// ---------------- QuickSort trên mảng ----------------

void Sorter::quickSortRooms(DynArray<PhongTro*>& a, RoomSortKey key, bool asc) {
    quickSort(a, [key, asc](PhongTro* x, PhongTro* y) {
        long long kx = roomKey(*x, key), ky = roomKey(*y, key);
        if (kx != ky) return asc ? kx < ky : kx > ky;
        return x->maPhong < y->maPhong;
    });
}

static long long billKey(const PhieuThuTien& b, BillSortKey key) {
    switch (key) {
    case BILL_BY_THANG: return (long long)b.nam * 100 + b.thang;
    case BILL_BY_TONG_TIEN: return b.tongTien;
    default: return b.maPhieuThuTien;
    }
}

void Sorter::quickSortBills(DynArray<PhieuThuTien*>& a, BillSortKey key, bool asc) {
    quickSort(a, [key, asc](PhieuThuTien* x, PhieuThuTien* y) {
        long long kx = billKey(*x, key), ky = billKey(*y, key);
        if (kx != ky) return asc ? kx < ky : kx > ky;
        return x->maPhieuThuTien < y->maPhieuThuTien;
    });
}

// ---------------- Binary Search ----------------

int Sorter::lowerBound(const DynArray<PhongTro*>& a, RoomSortKey key, long long value) {
    int lo = 0, hi = a.size();
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (roomKey(*a[mid], key) < value) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

int Sorter::upperBound(const DynArray<PhongTro*>& a, RoomSortKey key, long long value) {
    int lo = 0, hi = a.size();
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (roomKey(*a[mid], key) <= value) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

void Sorter::searchRooms(const RoomList& list, const RoomFilter& f, DynArray<PhongTro*>& out) {
    out.clear();
    DynArray<PhongTro*> all;
    for (RoomNode* n = list.head(); n; n = n->next) all.push(&n->data);

    quickSortRooms(all, ROOM_BY_GIA, true);
    int from = f.giaMin >= 0 ? lowerBound(all, ROOM_BY_GIA, f.giaMin) : 0;
    int to = f.giaMax >= 0 ? upperBound(all, ROOM_BY_GIA, f.giaMax) : all.size();

    for (int i = from; i < to; ++i) {
        const PhongTro* p = all[i];
        if (f.dtMin >= 0 && p->dienTich < f.dtMin) continue;
        if (f.dtMax >= 0 && p->dienTich > f.dtMax) continue;
        if (f.chiPhongTrong && p->trangThai != PHONG_TRONG) continue;
        out.push(all[i]);
    }
}

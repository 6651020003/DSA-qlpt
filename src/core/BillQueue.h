// BillQueue.h - [TV4] Hàng đợi ưu tiên (Min-Heap) quản lý Hóa đơn chưa thanh toán
// Độ ưu tiên: hạn thanh toán sớm nhất (nợ lâu nhất) lên đầu;
// cùng hạn thì tổng tiền lớn hơn ưu tiên trước; cùng tiền thì mã phiếu nhỏ hơn trước.
#ifndef BILL_QUEUE_H
#define BILL_QUEUE_H

#include "Models.h"
#include "DynArray.h"

class BillQueue {
public:
    BillQueue();
    ~BillQueue();
    BillQueue(const BillQueue&) = delete;
    BillQueue& operator=(const BillQueue&) = delete;

    void push(PhieuThuTien* bill);           // O(log n)
    PhieuThuTien* top() const;               // O(1)
    PhieuThuTien* pop();                     // O(log n)
    bool remove(PhieuThuTien* bill);         // O(log n) nhờ heapIndex lưu trong phiếu
    void update(PhieuThuTien* bill);         // gọi khi hạn/tổng tiền thay đổi
    bool contains(const PhieuThuTien* bill) const;

    int size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    void clear();

    // Xuất toàn bộ phiếu theo đúng thứ tự ưu tiên (heap sort trên bản sao, không đổi heap gốc)
    void toSortedArray(DynArray<PhieuThuTien*>& out) const;

    static bool higherPriority(const PhieuThuTien* a, const PhieuThuTien* b);

private:
    PhieuThuTien** m_heap;
    int m_size;
    int m_cap;

    void grow();
    void place(int i, PhieuThuTien* b);
    void siftUp(int i);
    void siftDown(int i);
};

#endif

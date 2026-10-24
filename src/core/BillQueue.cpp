#include "BillQueue.h"

BillQueue::BillQueue() : m_heap(nullptr), m_size(0), m_cap(0) {}

BillQueue::~BillQueue() { delete[] m_heap; }

bool BillQueue::higherPriority(const PhieuThuTien* a, const PhieuThuTien* b) {
    int c = a->hanThanhToan.compare(b->hanThanhToan);
    if (c != 0) return c < 0;
    if (a->tongTien != b->tongTien) return a->tongTien > b->tongTien;
    return a->maPhieuThuTien < b->maPhieuThuTien;
}

void BillQueue::grow() {
    int nc = m_cap == 0 ? 16 : m_cap * 2;
    PhieuThuTien** nh = new PhieuThuTien*[nc];
    for (int i = 0; i < m_size; ++i) nh[i] = m_heap[i];
    delete[] m_heap;
    m_heap = nh;
    m_cap = nc;
}

void BillQueue::place(int i, PhieuThuTien* b) {
    m_heap[i] = b;
    b->heapIndex = i;
}

void BillQueue::siftUp(int i) {
    PhieuThuTien* b = m_heap[i];
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (!higherPriority(b, m_heap[parent])) break;
        place(i, m_heap[parent]);
        i = parent;
    }
    place(i, b);
}

void BillQueue::siftDown(int i) {
    PhieuThuTien* b = m_heap[i];
    for (;;) {
        int l = 2 * i + 1, r = l + 1, best = i;
        PhieuThuTien* bestBill = b;
        if (l < m_size && higherPriority(m_heap[l], bestBill)) { best = l; bestBill = m_heap[l]; }
        if (r < m_size && higherPriority(m_heap[r], bestBill)) { best = r; bestBill = m_heap[r]; }
        if (best == i) break;
        place(i, m_heap[best]);
        i = best;
    }
    place(i, b);
}

void BillQueue::push(PhieuThuTien* bill) {
    if (!bill || contains(bill)) return;
    if (m_size == m_cap) grow();
    place(m_size, bill);
    siftUp(m_size++);
}

PhieuThuTien* BillQueue::top() const {
    return m_size > 0 ? m_heap[0] : nullptr;
}

PhieuThuTien* BillQueue::pop() {
    if (m_size == 0) return nullptr;
    PhieuThuTien* t = m_heap[0];
    t->heapIndex = -1;
    --m_size;
    if (m_size > 0) {
        place(0, m_heap[m_size]);
        siftDown(0);
    }
    return t;
}

bool BillQueue::contains(const PhieuThuTien* bill) const {
    return bill && bill->heapIndex >= 0 && bill->heapIndex < m_size && m_heap[bill->heapIndex] == bill;
}

bool BillQueue::remove(PhieuThuTien* bill) {
    if (!contains(bill)) return false;
    int i = bill->heapIndex;
    bill->heapIndex = -1;
    --m_size;
    if (i == m_size) return true;  // phần tử cuối
    // Đưa phần tử cuối vào chỗ trống rồi vun lại (có thể phải đi lên hoặc đi xuống)
    PhieuThuTien* last = m_heap[m_size];
    place(i, last);
    siftUp(i);
    siftDown(last->heapIndex);
    return true;
}

void BillQueue::update(PhieuThuTien* bill) {
    if (!contains(bill)) return;
    int i = bill->heapIndex;
    siftUp(i);
    siftDown(bill->heapIndex);
}

void BillQueue::clear() {
    for (int i = 0; i < m_size; ++i) m_heap[i]->heapIndex = -1;
    m_size = 0;
}

void BillQueue::toSortedArray(DynArray<PhieuThuTien*>& out) const {
    out.clear();
    if (m_size == 0) return;
    // Sao chép mảng heap rồi lấy ra lần lượt phần tử ưu tiên nhất
    PhieuThuTien** h = new PhieuThuTien*[m_size];
    int n = m_size;
    for (int i = 0; i < n; ++i) h[i] = m_heap[i];
    while (n > 0) {
        out.push(h[0]);
        h[0] = h[--n];
        int i = 0;
        for (;;) {
            int l = 2 * i + 1, r = l + 1, best = i;
            if (l < n && higherPriority(h[l], h[best])) best = l;
            if (r < n && higherPriority(h[r], h[best])) best = r;
            if (best == i) break;
            PhieuThuTien* t = h[i];
            h[i] = h[best];
            h[best] = t;
            i = best;
        }
    }
    delete[] h;
}

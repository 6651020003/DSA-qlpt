// DynArray.h - Mảng động tự cài đặt (cấp phát động, tự tăng dung lượng x2)
#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

template <typename T>
class DynArray {
public:
    DynArray() : m_data(nullptr), m_size(0), m_cap(0) {}

    DynArray(const DynArray& o) : m_data(nullptr), m_size(0), m_cap(0) {
        reserve(o.m_size);
        for (int i = 0; i < o.m_size; ++i) m_data[i] = o.m_data[i];
        m_size = o.m_size;
    }

    DynArray& operator=(const DynArray& o) {
        if (this != &o) {
            m_size = 0;
            reserve(o.m_size);
            for (int i = 0; i < o.m_size; ++i) m_data[i] = o.m_data[i];
            m_size = o.m_size;
        }
        return *this;
    }

    ~DynArray() { delete[] m_data; }

    void reserve(int cap) {
        if (cap <= m_cap) return;
        T* nd = new T[cap];
        for (int i = 0; i < m_size; ++i) nd[i] = m_data[i];
        delete[] m_data;
        m_data = nd;
        m_cap = cap;
    }

    void push(const T& v) {
        if (m_size == m_cap) reserve(m_cap == 0 ? 8 : m_cap * 2);
        m_data[m_size++] = v;
    }

    // Xóa phần tử tại i, dồn các phần tử phía sau lên (giữ thứ tự)
    void removeAt(int i) {
        if (i < 0 || i >= m_size) return;
        for (int k = i; k + 1 < m_size; ++k) m_data[k] = m_data[k + 1];
        --m_size;
    }

    void swap(int i, int j) {
        T t = m_data[i];
        m_data[i] = m_data[j];
        m_data[j] = t;
    }

    void clear() { m_size = 0; }
    int size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    T& operator[](int i) { return m_data[i]; }
    const T& operator[](int i) const { return m_data[i]; }

private:
    T* m_data;
    int m_size;
    int m_cap;
};

#endif

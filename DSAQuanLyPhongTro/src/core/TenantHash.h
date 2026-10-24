// TenantHash.h - [TV3] Bảng băm kết hợp DSLK đơn (Separate Chaining)
// Quản lý Khách thuê (theo Mã KH / CCCD / SĐT) và Tài khoản (theo Mã TK / UserName)
#ifndef TENANT_HASH_H
#define TENANT_HASH_H

#include <string>
#include "Models.h"
#include "DynArray.h"

unsigned long hashKey(int key);
unsigned long hashKey(const std::string& key);
int nextPrime(int n);

template <typename K, typename V>
class HashTable {
public:
    struct Entry {
        K key;
        V value;
        Entry* next;
        Entry(const K& k, const V& v, Entry* n) : key(k), value(v), next(n) {}
    };

    explicit HashTable(int capacity = 31) : m_cap(capacity), m_size(0) {
        m_buckets = new Entry*[m_cap];
        for (int i = 0; i < m_cap; ++i) m_buckets[i] = nullptr;
    }

    ~HashTable() {
        clear();
        delete[] m_buckets;
    }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    // Thêm vào đầu chuỗi của bucket - O(1) trung bình. Trả về false nếu khóa đã tồn tại.
    bool insert(const K& key, const V& value) {
        if (find(key)) return false;
        if ((m_size + 1) * 4 > m_cap * 3) rehash(nextPrime(m_cap * 2));  // hệ số tải > 0.75
        int b = indexOf(key, m_cap);
        m_buckets[b] = new Entry(key, value, m_buckets[b]);
        ++m_size;
        return true;
    }

    V* find(const K& key) const {
        for (Entry* e = m_buckets[indexOf(key, m_cap)]; e; e = e->next)
            if (e->key == key) return &e->value;
        return nullptr;
    }

    bool remove(const K& key) {
        Entry** pp = &m_buckets[indexOf(key, m_cap)];
        while (*pp) {
            if ((*pp)->key == key) {
                Entry* del = *pp;
                *pp = del->next;
                delete del;
                --m_size;
                return true;
            }
            pp = &(*pp)->next;
        }
        return false;
    }

    void clear() {
        for (int i = 0; i < m_cap; ++i) {
            Entry* e = m_buckets[i];
            while (e) {
                Entry* next = e->next;
                delete e;
                e = next;
            }
            m_buckets[i] = nullptr;
        }
        m_size = 0;
    }

    template <typename F>
    void forEach(F f) const {
        for (int i = 0; i < m_cap; ++i)
            for (Entry* e = m_buckets[i]; e; e = e->next) f(e->key, e->value);
    }

    int size() const { return m_size; }
    int capacity() const { return m_cap; }

    int longestChain() const {
        int best = 0;
        for (int i = 0; i < m_cap; ++i) {
            int len = 0;
            for (Entry* e = m_buckets[i]; e; e = e->next) ++len;
            if (len > best) best = len;
        }
        return best;
    }

private:
    Entry** m_buckets;
    int m_cap;
    int m_size;

    static int indexOf(const K& key, int cap) { return (int)(hashKey(key) % (unsigned long)cap); }

    void rehash(int newCap) {
        Entry** nb = new Entry*[newCap];
        for (int i = 0; i < newCap; ++i) nb[i] = nullptr;
        for (int i = 0; i < m_cap; ++i) {
            Entry* e = m_buckets[i];
            while (e) {
                Entry* next = e->next;
                int b = indexOf(e->key, newCap);
                e->next = nb[b];
                nb[b] = e;
                e = next;
            }
        }
        delete[] m_buckets;
        m_buckets = nb;
        m_cap = newCap;
    }
};

class TenantHash {
public:
    TenantHash();
    ~TenantHash();
    TenantHash(const TenantHash&) = delete;
    TenantHash& operator=(const TenantHash&) = delete;

    // ----- Khách hàng (bảng chính theo Mã KH, chỉ mục phụ theo CCCD và SĐT) -----
    bool addCustomer(const KhachHang& kh);      // false nếu trùng Mã / CCCD / SĐT
    bool updateCustomer(const KhachHang& kh);   // cập nhật theo Mã, tự sửa chỉ mục phụ
    bool removeCustomer(int maKhachHang);
    KhachHang* findCustomer(int maKhachHang) const;
    KhachHang* findByCCCD(const std::string& cccd) const;
    KhachHang* findBySDT(const std::string& sdt) const;
    KhachHang* findByAccount(int maTaiKhoan) const;
    void getCustomers(DynArray<KhachHang*>& out) const;
    int customerCount() const { return m_byMa.size(); }
    int maxCustomerId() const;

    // ----- Tài khoản (bảng theo Mã TK và theo UserName) -----
    bool addAccount(const TaiKhoan& tk);         // false nếu trùng Mã / UserName
    bool removeAccount(int maTaiKhoan);
    TaiKhoan* findAccount(int maTaiKhoan) const;
    TaiKhoan* findAccount(const std::string& userName) const;
    void getAccounts(DynArray<TaiKhoan*>& out) const;
    int accountCount() const { return m_accById.size(); }
    int maxAccountId() const;

    // Thông số bảng băm (hiển thị trên giao diện / báo cáo)
    int customerBuckets() const { return m_byMa.capacity(); }
    int customerLongestChain() const { return m_byMa.longestChain(); }

    void clear();

private:
    HashTable<int, KhachHang*> m_byMa;
    HashTable<std::string, KhachHang*> m_byCCCD;
    HashTable<std::string, KhachHang*> m_bySDT;
    HashTable<int, TaiKhoan*> m_accById;
    HashTable<std::string, TaiKhoan*> m_accByUser;
};

#endif

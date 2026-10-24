// RoomList.h - [TV3] Danh sách liên kết đôi quản lý Phòng trọ
#ifndef ROOM_LIST_H
#define ROOM_LIST_H

#include "Models.h"

struct RoomNode {
    PhongTro data;
    RoomNode* prev;
    RoomNode* next;

    explicit RoomNode(const PhongTro& p) : data(p), prev(nullptr), next(nullptr) {}
};

class RoomList {
public:
    RoomList();
    ~RoomList();
    RoomList(const RoomList&) = delete;
    RoomList& operator=(const RoomList&) = delete;

    RoomNode* head() const { return m_head; }
    RoomNode* tail() const { return m_tail; }
    int size() const { return m_size; }

    RoomNode* find(int maPhong) const;
    bool pushBack(const PhongTro& p);   // false nếu trùng mã phòng
    bool pushFront(const PhongTro& p);
    bool update(const PhongTro& p);     // cập nhật theo mã phòng
    bool remove(int maPhong);
    void clear();
    int countByStatus(int trangThai) const;

private:
    RoomNode* m_head;
    RoomNode* m_tail;
    int m_size;

    void unlink(RoomNode* n);

    friend class Sorter;  // MergeSort nối lại con trỏ trực tiếp
};

#endif

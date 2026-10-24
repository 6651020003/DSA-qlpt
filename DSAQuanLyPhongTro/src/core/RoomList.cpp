#include "RoomList.h"

RoomList::RoomList() : m_head(nullptr), m_tail(nullptr), m_size(0) {}

RoomList::~RoomList() { clear(); }

RoomNode* RoomList::find(int maPhong) const {
    for (RoomNode* n = m_head; n; n = n->next)
        if (n->data.maPhong == maPhong) return n;
    return nullptr;
}

bool RoomList::pushBack(const PhongTro& p) {
    if (find(p.maPhong)) return false;
    RoomNode* n = new RoomNode(p);
    n->prev = m_tail;
    if (m_tail) m_tail->next = n;
    else m_head = n;
    m_tail = n;
    ++m_size;
    return true;
}

bool RoomList::pushFront(const PhongTro& p) {
    if (find(p.maPhong)) return false;
    RoomNode* n = new RoomNode(p);
    n->next = m_head;
    if (m_head) m_head->prev = n;
    else m_tail = n;
    m_head = n;
    ++m_size;
    return true;
}

bool RoomList::update(const PhongTro& p) {
    RoomNode* n = find(p.maPhong);
    if (!n) return false;
    n->data = p;
    return true;
}

void RoomList::unlink(RoomNode* n) {
    if (n->prev) n->prev->next = n->next;
    else m_head = n->next;
    if (n->next) n->next->prev = n->prev;
    else m_tail = n->prev;
    --m_size;
}

bool RoomList::remove(int maPhong) {
    RoomNode* n = find(maPhong);
    if (!n) return false;
    unlink(n);
    delete n;
    return true;
}

void RoomList::clear() {
    RoomNode* n = m_head;
    while (n) {
        RoomNode* next = n->next;
        delete n;
        n = next;
    }
    m_head = m_tail = nullptr;
    m_size = 0;
}

int RoomList::countByStatus(int trangThai) const {
    int c = 0;
    for (RoomNode* n = m_head; n; n = n->next)
        if (n->data.trangThai == trangThai) ++c;
    return c;
}

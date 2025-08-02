module;

#include <interrupt.h>

export module list;

struct node
{
    node* prev;
    node* next;
};

export struct list
{
    auto insert(node* it,node* v) -> list&
    {
        auto old_stu = intr_disable();
        it->prev->next = v;
        v->prev = it->prev;
        v->next = v;
        it->prev = v;
        ++sz;
        intr_set_status(old_stu);
        return *this;
    }

    auto push_front(node* v) -> list&
    {
        insert(front(),v);
        return *this;
    }

    auto push_back(node* v) -> list&
    {
        insert(back(),v);
        return *this;
    }

    auto erase(node* v) -> list&
    {
        auto old_stu = intr_disable();
        v->prev->next = v->next;
        v->next->prev = v->prev;
        --sz;
        intr_set_status(old_stu);
        return *this;
    }

    auto front() -> node*
    {
        return head.next;
    }

    auto pop_front() -> list&
    {
        erase(front());
        return *this;
    }

    auto back() -> node*
    {
        return tail.prev;
    }

    auto pop_back() -> list&
    {
        erase(back());
        return *this;
    }

    auto begin() -> node*
    {
        return head.next;
    }

    auto end() -> node*
    {
        return &tail;
    }

    auto contains(node* v) const -> bool
    {
        for(auto it : *this) {
            if(&it == v) {
                return true;
            }
        }
        return false;
    }

    auto empty() const -> bool
    {
        return not size();
    }

    auto size() const -> size_t
    {
        return sz;
    }

    template<typename Pred>
    auto find(Pred pred) const -> node*
    {
        for(auto it : *this) {
            if(pred(&it)) {
                return &it;
            }
        }
        return nullptr;
    }

    // head->prev 用作 bound 第一个元素实为head->next
    // tail->next 用作 bound
    size_t sz = 0;
    node head{ nullptr,&tail };
    node tail{ &head,nullptr };
};


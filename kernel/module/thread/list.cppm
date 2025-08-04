module;

#include <interrupt.h>
#include <stdio.h>

module thread:list;

import utility;

struct list
{
    struct node
    {
        node* prev;
        node* next;
    };

    constexpr list() : sz(0),head{ nullptr,&tail },tail{ &head,nullptr }
    {
        puts("list construction! *************     \n");
    }

    auto insert(node* it,node* v) -> list&
    {
        auto old_stu = intr_disable();
        it->prev->next = v;
        v->prev = it->prev;
        v->next = it;
        it->prev = v;
        ++sz;
        intr_set_status(old_stu);
        return *this;
    }

    auto push_front(node* v) -> list&
    {
        insert(begin(),v);
        return *this;
    }

    auto push_back(node* v) -> list&
    {
        insert(end(),v);
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

    auto contains(node* v) -> bool
    {
        for(auto it = begin(); it != end(); it = it->next) {
            if(it == v) {
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
    auto find(Pred pred) -> node*
    {
        for(auto it = begin(); it != end(); it = it->next) {
            if(pred(it)) {
                return it;
            }
        }
        return nullptr;
    }

    // head->prev 用作 bound 第一个元素实为head->next
    // tail->next 用作 bound
    size_t sz;
    node head;
    node tail;
};


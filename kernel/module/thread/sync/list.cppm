module;

#include <interrupt.h>
#include <stdio.h>

export module list;

import utility;

export struct thread_list
{
    struct node
    {
        node* prev;
        node* next;
    };

    constexpr thread_list() : sz(0),head{ nullptr,&tail },tail{ &head,nullptr }
    {
        puts("thread_list construction! *************     \n");
    }

    auto insert(node* it,node* v) -> thread_list&
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

    auto push_front(node* v) -> thread_list&
    {
        insert(begin(),v);
        return *this;
    }

    auto push_back(node* v) -> thread_list&
    {
        insert(end(),v);
        return *this;
    }

    auto erase(node* v) -> thread_list&
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

    auto pop_front() -> thread_list&
    {
        erase(front());
        return *this;
    }

    auto back() -> node*
    {
        return tail.prev;
    }

    auto pop_back() -> thread_list&
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


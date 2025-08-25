module;

#include <interrupt.h>
#include <stdio.h>

export module list;

import utility;

export struct list
{
    struct node
    {
        node* prev;
        node* next;
    };

    struct iter : std::iter::forward<node>
    {
        auto constexpr operator++(this iter& self) -> iter&
        {
            auto& it = self.it;
            it = it->next;
            return self;
        }

        auto constexpr operator--(this iter& self) -> iter&
        {
            auto& it = self.it;
            it = it->prev;
            return self;
        }
    };

    using value_type = node;
    using iterator = iter;

    constexpr list()
    {
        init();
    }

    auto init() -> void
    {
        sz = 0;
        head = { nullptr,&tail };
        tail = { &head, nullptr };
    }

    list(list const&) = delete("The list do not need copy constructor, because it can cause some amazing error!");

    auto operator=(list const&) -> list& = delete("too");

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

    template<typename Pred>
    auto for_each(Pred pred) -> void
    {
        for(auto it = begin(); it != end(); it = it->next) {
            pred(it);
        }
    }

    // head->prev 用作 bound 第一个元素实为head->next
    // tail->next 用作 bound
    size_t sz;
    node head;
    node tail;
};


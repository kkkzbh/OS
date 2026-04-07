module;

#include <interrupt.h>
#include <stdio.h>
#include <assert.h>

export module list;

export import list.node;

import utility;

export struct list
{
    using node = ::list_node;

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

        operator node*() const
        {
            return it;
        }
    };

    using value_type = node;
    using iterator = iter;

    auto constexpr init() -> void
    {
        head = { nullptr,&tail };
        tail = { &head, nullptr };
    }

    auto operator=(list const&) -> list& = delete("too");

    auto insert(node* it,node* v) -> list&
    {
        assert_initialized();
        auto old_stu = intr_disable();
        it->prev->next = v;
        v->prev = it->prev;
        v->next = it;
        it->prev = v;
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

    auto static erase(node* v) -> void
    {
        ASSERT(v != nullptr);
        ASSERT(v->prev != nullptr);
        ASSERT(v->next != nullptr);
        auto old_stu = intr_disable();
        v->prev->next = v->next;
        v->next->prev = v->prev;
        intr_set_status(old_stu);
    }

    auto front() -> node*
    {
        assert_initialized();
        return head.next;
    }

    auto pop_front() -> list&
    {
        erase(front());
        return *this;
    }

    auto back() -> node*
    {
        assert_initialized();
        return tail.prev;
    }

    auto pop_back() -> list&
    {
        erase(back());
        return *this;
    }

    auto begin() -> iterator
    {
        assert_initialized();
        return { head.next };
    }

    auto end() -> iterator
    {
        assert_initialized();
        return { &tail };
    }

    auto contains(node const* v) -> bool
    {
        for(auto const& nd : *this) {
            if(&nd == v) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    auto empty() const -> bool
    {
        assert_initialized();
        return head.next == &tail;
    }

    auto assert_initialized() const -> void
    {
        ASSERT(is_initialized());
    }

    auto is_initialized() const -> bool
    {
        return head.next != nullptr and tail.prev != nullptr;
    }

    // head->prev 用作 bound 第一个元素实为head->next
    // tail->next 用作 bound
    node head;
    node tail;
};

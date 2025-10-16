

export module scope;

import utility;

export template<typename F>
struct scope_exit
{

    template<typename Lam>
    explicit scope_exit(Lam&& fn,bool const& act) : f(std::forward<Lam>(fn)),active(act) {}

    ~scope_exit()
    {
        if(active) {
            f();
        }
    }

    scope_exit(scope_exit const&) = delete("不应该被拷贝，就像f不应该被调用两次那样");
    auto operator=(scope_exit const&) -> scope_exit& = delete;

    F f;
    bool const& active;
};

export template<typename Lam>
scope_exit(Lam,bool const&) -> scope_exit<Lam>;
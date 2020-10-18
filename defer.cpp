
// FROM: https://stackoverflow.com/questions/50182244/simple-way-to-execute-code-at-the-end-of-function-scope

template <typename FUNC>
struct deferred_call
{
    // Disallow assignment and copy


    deferred_call(const deferred_call& that) = delete;
    deferred_call& operator=(const deferred_call& that) = delete;

    // Pass in a lambda

    deferred_call(FUNC&& f) 
        : m_func(std::forward<FUNC>(f)), m_bOwner(true) 
    {
    }

    // Move constructor, since we disallow the copy

    deferred_call(deferred_call&& that)
        : m_func(std::move(that.m_func)), m_bOwner(that.m_bOwner)
    {
        that.m_bOwner = false;
    }

    // Destructor forces deferred call to be executed

    ~deferred_call()
    {
        execute();
    }

    // Prevent the deferred call from ever being invoked

    bool cancel()
    {
        bool bWasOwner = m_bOwner;
        m_bOwner = false;
        return bWasOwner;
    }

    // Cause the deferred call to be invoked NOW

    bool execute()
    {
        const auto bWasOwner = m_bOwner;

        if (m_bOwner)
        {
            m_bOwner = false;
            m_func();
        }

        return bWasOwner;
    }

private:
    FUNC m_func;
    bool m_bOwner;
};

template <typename F>
deferred_call<F> defer_(F&& f)
{
    return deferred_call<F>(std::forward<F>(f));
}


#define defer__concat_(x, y) x##y
#define defer__concat(x, y) defer__concat_(x, y)
#define defer(code) auto defer__concat(__DEFER__, __COUNTER__) = defer_([&]{code});

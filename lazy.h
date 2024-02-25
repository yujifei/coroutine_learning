#pragma once
#include <coroutine>
#include <string>

template<typename T = void> class Lazy
{
public:
    class promise_type
    {
    public:
        promise_type(const std::string&, const std::string&)
        {
        }
        Lazy<T> get_return_object()
        {
            return Lazy{ CoHandle::from_promise(*this) };
        }
        static std::suspend_never initial_suspend()
        {
            return {};
        }
        static std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        std::suspend_always yield_value(T value)
        {
            m_value = std::move(value);
            return {};
        }
        void return_value(T value)
        {
            m_value = std::move(value);
        }
        void unhandled_exception() { throw; }

        T& get_value()
        {
            return m_value;
        }

    private:
        T m_value;
    };

    using CoHandle = std::coroutine_handle<promise_type>;

    Lazy(CoHandle handle) : m_handle(handle)
    {
    }
    Lazy(const Lazy&) = delete;
    Lazy& operator=(const Lazy&) = delete;
    Lazy(Lazy&& other) : m_handle(other.m_handle)
    {
        other.m_handle = {};
    }
    Lazy& operator=(Lazy&& other)
    {
        if (this != &other)
        {
            if (m_handle)
                m_handle.destroy();
            m_handle = other.m_handle;
            other.m_handle = {};
        }
        return *this;
    }
    ~Lazy()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

private:
    CoHandle m_handle;
};

template<> class Lazy<void>::promise_type {
public:
    promise_type(const std::string&, const std::string&)
    {
    }
    ~promise_type()
    {
    }
    Lazy<void> get_return_object()
    {
        return Lazy{ CoHandle::from_promise(*this) };
    }
    std::suspend_never initial_suspend()
    {
        return {};
    }
    std::suspend_always final_suspend() noexcept
    {
        return {};
    }
    void return_void()
    {
    }
    void unhandled_exception() { throw; }
};

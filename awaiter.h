#pragma once
#include "executor.h"
#include "log.h"
#include <type_traits>

template<typename F, typename... Args> class CommonAwaiter
{
public:
    using ReturnType = std::decay_t<std::invoke_result_t<F, Args...>>;
    using WorkerType = std::function<ReturnType(void)>;
public:
    CommonAwaiter(F func, Args... args)
    {
        m_worker = [func, args...]()->ReturnType
        {
            return std::invoke(func, args...);
        };
    }
    ~CommonAwaiter()
    {
    }
    bool await_ready()
    {
        return false;
    }
    bool await_suspend(std::coroutine_handle<> handle)
    {
        if constexpr (std::is_same_v<ReturnType, void>)
        {
            auto cb = [handle, this]()
            {
                info("task finished wakeup coroutine");
                Executor::instance().postMainTask([handle, this]()->void
                {
                    handle.resume();
                });
            };
            Executor::instance().postThreadTask(m_worker, cb);
        }
        else
        {
            auto cb = [handle, this](ReturnType value)
            {
                Executor::instance().postMainTask([handle, this, v = std::move(value)]()->void
                {
                    m_data_ptr = std::make_shared<ReturnType>(std::move(v));
                    handle.resume();
                });
            };
            Executor::instance().postThreadTask(m_worker, cb);
        }
        info("coroutine suspend");
        return true;
    }
    ReturnType await_resume()
    {
        info("coroutine resume");
        if constexpr(!std::is_same_v<ReturnType, void>)
            return std::move(*m_data_ptr);
    }

private:
    std::shared_ptr<ReturnType> m_data_ptr;
    WorkerType m_worker;
};

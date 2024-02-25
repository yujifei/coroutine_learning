#pragma once

#include <memory>
#include <future>
#include <mutex>
#include <list>
#include <functional>
#include <condition_variable>
#include <type_traits>

using ExecTask = std::function<void()>;

struct Task 
{
    unsigned int task_id = 0;
    ExecTask task;
};

template<typename F> concept return_void =
requires(F f)
{
    std::is_same_v<void, std::invoke_result_t<F>>;
};

template<typename C> concept accept_void =
requires(C c)
{
    c();
};

template<typename F, typename C> concept chainable =
requires(F f, C c)
{
    c(std::declval<std::invoke_result_t<F>>());
};

template<typename F, typename C> concept callbackable = accept_void<F> && (return_void<F> && accept_void<C> || chainable<F, C>);


class Executor
{
public:
    static Executor& instance();
    ~Executor();
    template<typename F, typename C> requires callbackable<F, C> unsigned int postThreadTask(F&& f, C&& c);
    template<typename F> requires accept_void<F>&& return_void<F> unsigned int postThreadTask(F&& f);
    template<typename F, typename C> requires callbackable<F, C> void postMainTask(F&& f, C&& c);
    template<typename F> requires accept_void<F> && return_void<F> void postMainTask(F&& f);
    bool cancelThreadTask(unsigned int task_id);
    void run();
    void quit();

private:
    Executor(int n_thread = 0);
    Executor(const Executor&) = delete;
    Executor(Executor&&) = delete;
    Executor& operator=(const Executor&) = delete;
    Executor& operator=(Executor&&) = delete;

    void threadFunc();

private:
    std::list<Task> m_thread_task_list;
    std::list<ExecTask> m_main_task_list;

    unsigned int m_cur_task_id = 0;
    std::atomic_bool m_running{true};
    std::condition_variable m_thread_condition;
    std::mutex m_thread_task_lock;
    std::condition_variable m_main_condition;
    std::mutex m_main_task_lock;
    std::vector<std::thread> m_thread_pool;
    std::thread::id m_main_thread;
};

template<typename F, typename C> requires callbackable<F, C> unsigned int Executor::postThreadTask(F&& f, C&& c)
{
    auto taskFunc = [func = std::move(f), cb = std::move(c)]()
    {
        if constexpr(std::is_same_v<void, std::invoke_result_t<decltype(func)>>)
        {
            func();
            cb();
        }
        else
        {
            cb(func());
        }
    };
    unsigned int my_id = 0;
    {
        std::lock_guard<std::mutex> lg(m_thread_task_lock);
        ++m_cur_task_id;
        Task task{ m_cur_task_id, std::move(taskFunc) };
        m_thread_task_list.emplace_back(std::move(task));
        my_id = m_cur_task_id;
    }
    m_thread_condition.notify_one();
    return my_id;
}

template<typename F> requires accept_void<F> && return_void<F> unsigned int Executor::postThreadTask(F&& f) 
{
    unsigned int my_id = 0;
    {
        std::lock_guard<std::mutex> lg(m_thread_task_lock);
        ++m_cur_task_id;
        Task task{ m_cur_task_id, std::move(f) };
        m_thread_task_list.emplace_back(std::move(task));
        my_id = m_cur_task_id;
    }
    m_thread_condition.notify_one();
    return my_id;
}

template<typename F, typename C> requires callbackable<F, C> void Executor::postMainTask(F&& f, C&& c)
{
    auto task = [func = std::move(f), cb = std::move(c)]
    {
        if constexpr (std::is_same<std::invoke_result_t<decltype(func)>, void>::value)
        {
            func();
            cb();
        }
        else
        {
            cb(func());
        }
    };
    {
        std::lock_guard<std::mutex> lg(m_main_task_lock);
        m_main_task_list.emplace_back(std::move(task));
    }
    m_main_condition.notify_one();
}

template<typename F> requires accept_void<F>&& return_void<F> void Executor::postMainTask(F&& f)
{
    {
        std::lock_guard<std::mutex> lg(m_main_task_lock);
        m_main_task_list.emplace_back(std::move(f));
    }
    m_main_condition.notify_one();
}
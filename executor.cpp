#include "executor.h"
#include "log.h"
#include <chrono>

using namespace std::chrono_literals;

Executor& Executor::instance()
{
    static Executor s_instance;
    return s_instance;
}

Executor::Executor(int n_thread)
{
    if (n_thread <= 0)
    {
        n_thread = std::thread::hardware_concurrency();
    }
    for (int i = 0; i < n_thread; ++i)
    {
        std::thread worker(std::bind(&Executor::threadFunc, this));
        m_thread_pool.emplace_back(std::move(worker));
    }
    info("%d thread started", n_thread);
}

Executor::~Executor() 
{
    m_running = false;
    m_thread_condition.notify_all();
    for (auto& t : m_thread_pool)
    {
        t.join();
    }
}

bool Executor::cancelThreadTask(unsigned int task_id)
{
    std::unique_lock<std::mutex> ul(m_thread_task_lock);
    auto iter = std::find_if(m_thread_task_list.begin(), m_thread_task_list.end(), [task_id](const auto& t) { return t.task_id == task_id; });
    if (iter == m_thread_task_list.end())
    {
        return false;
    }
    m_thread_task_list.erase(iter);
    return false;
}

void Executor::threadFunc()
{
    while (true)
    {
        Task task{};
        {
            std::unique_lock<std::mutex> ul(m_thread_task_lock);
            m_thread_condition.wait(ul, [this] {return !m_running || !m_thread_task_list.empty(); });
            if (!m_running) {
                break;
            }
            if (!m_thread_task_list.empty())
            {
                task = std::move(m_thread_task_list.front());
                m_thread_task_list.pop_front();
            }
        }
        std::invoke(task.task);
    }
}

void Executor::run() 
{
    while (true) 
    {
        std::this_thread::sleep_for(10ms);
        ExecTask task{};
        {
            std::unique_lock<std::mutex> ul(m_main_task_lock);
            m_main_condition.wait(ul, [this] {return !m_running || !m_main_task_list.empty(); });
            if (!m_running) {
                break;
            }
            if (m_main_task_list.empty())
            {
                continue;
            }
            if (!m_main_task_list.empty())
            {
                task = std::move(m_main_task_list.front());
                m_main_task_list.pop_front();
            }
        }
        std::invoke(task);
    }
}

void Executor::quit() 
{
    m_running = false;
}